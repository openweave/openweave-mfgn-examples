/*
 *    Copyright (c) 2020 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "WDMFeature.h"

#include "nrf.h" // FIXME

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>

using namespace ::nl;
using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::DeviceLayer::Internal;
using namespace ::nl::Weave::Profiles::DataManagement_Current;

// TODO: Remove this
#define kServiceEndpoint_Data_Management 0x18B4300200000003ull ///< Core Weave data management protocol endpoint

/** Defines the timeout for liveness between the service and the device.
 *  For sleepy end node devices, this timeout will be much larger than the current
 *  value to preserve battery.
 */
#define SERVICE_LIVENESS_TIMEOUT_SEC 60 * 1 // 1 minute
#define OCSENSOR_LIVENESS_TIMEOUT_SEC 10    // 10 seconds

/** Defines the timeout for a response to any message initiated by the device to the service.
 *  This includes notifies, subscribe confirms, cancels and updates.
 *  This timeout is kept SERVICE_WRM_MAX_RETRANS x SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS + some buffer
 *  to account for latency in the message transmission through multiple hops.
 */
#define SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS 10000
#define OCSENSOR_MESSAGE_RESPONSE_TIMEOUT_MS 3000

/** Defines the timeout for a message to get retransmitted when no wrm ack or
 *  response has been heard back from the service. This timeout is kept larger
 *  for now since the message has to travel through multiple hops and service
 *  layers before actually making it to the actual receiver.
 *  @note
 *    WRM has an initial and active retransmission timeouts to interact with
 *    sleepy destination nodes. For the time being, the pinna WRM config would
 *    set both these timeouts to be the same based on the assumption that the pinna
 *    would not be interacting directly with a sleepy peer.
 */
#define SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS 2500
#define OCSENSOR_WRM_INITIAL_RETRANS_TIMEOUT_MS 800

#define SERVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS 2500
#define OCSENSOR_WRM_ACTIVE_RETRANS_TIMEOUT_MS 500

/** Define the maximum number of retransmissions in WRM
 */
#define SERVICE_WRM_MAX_RETRANS 3
#define OCSENSOR_WRM_MAX_RETRANS 3

/** Define the timeout for piggybacking a WRM acknowledgment message
 */
#define SERVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS 200
#define OCSENSOR_WRM_PIGGYBACK_ACK_TIMEOUT_MS 200

/** Defines the timeout for expecting a subscribe response after sending a subscribe request.
 *  This is meant to be a gross timeout - the MESSAGE_RESPONSE_TIMEOUT_MS will usually trip first
 *  to catch timeouts for each message in the subscribe request exchange.
 *  SUBSCRIPTION_RESPONSE_TIMEOUT_MS > Average no. of notifies during a subscription * MESSAGE_RESPONSE_TIMEOUT_MS
 */
#define SUBSCRIPTION_RESPONSE_TIMEOUT_MS 40000

const nl::Weave::WRMPConfig gWRMPConfigService = { SERVICE_WRM_INITIAL_RETRANS_TIMEOUT_MS, SERVICE_WRM_ACTIVE_RETRANS_TIMEOUT_MS,
                                                   SERVICE_WRM_PIGGYBACK_ACK_TIMEOUT_MS, SERVICE_WRM_MAX_RETRANS };

const nl::Weave::WRMPConfig gWRMPConfigOCSensor = { OCSENSOR_WRM_INITIAL_RETRANS_TIMEOUT_MS, OCSENSOR_WRM_ACTIVE_RETRANS_TIMEOUT_MS,
                                                    OCSENSOR_WRM_PIGGYBACK_ACK_TIMEOUT_MS, OCSENSOR_WRM_MAX_RETRANS };

WDMFeature WDMFeature::sWDMFeature;

SubscriptionEngine * SubscriptionEngine::GetInstance()
{
    return &(GetWDMFeature().mSubscriptionEngine);
}

int PublisherLock::Init()
{
    mRecursiveLock = xSemaphoreCreateRecursiveMutex();
    return ((mRecursiveLock == NULL) ? WEAVE_ERROR_NO_MEMORY : WEAVE_NO_ERROR);
}

WEAVE_ERROR PublisherLock::Lock()
{
    if (pdTRUE != xSemaphoreTakeRecursive((SemaphoreHandle_t) mRecursiveLock, portMAX_DELAY))
    {
        return WEAVE_ERROR_LOCKING_FAILURE;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR PublisherLock::Unlock()
{
    if (pdTRUE != xSemaphoreGiveRecursive((SemaphoreHandle_t) mRecursiveLock))
    {
        return WEAVE_ERROR_LOCKING_FAILURE;
    }

    return WEAVE_NO_ERROR;
}

WDMFeature::WDMFeature(void) :
    mServiceSinkTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mServiceSinkCatalogStore,
                             sizeof(mServiceSinkCatalogStore) / sizeof(mServiceSinkCatalogStore[0])),
    mOCSensorSinkTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mOCSensorSinkCatalogStore,
                              sizeof(mOCSensorSinkCatalogStore) / sizeof(mOCSensorSinkCatalogStore[0])),
    mServiceSourceTraitCatalog(ResourceIdentifier(ResourceIdentifier::SELF_NODE_ID), mServiceSourceCatalogStore,
                               sizeof(mServiceSourceCatalogStore) / sizeof(mServiceSourceCatalogStore[0])),

    mServiceSubClient(NULL), mServiceCounterSubHandler(NULL), mServiceSubBinding(NULL), mIsSubToServiceEstablished(false),
    mIsServiceCounterSubEstablished(false), mIsSubToServiceActivated(false),

    mOCSensorSubClient(NULL), mOCSensorSubBinding(NULL), mIsSubToOCSensorEstablished(false)
{}

void WDMFeature::AsyncProcessChanges(intptr_t arg)
{
    sWDMFeature.mSubscriptionEngine.GetNotificationEngine()->Run();
}

void WDMFeature::ProcessTraitChanges(void)
{
    PlatformMgr().ScheduleWork(AsyncProcessChanges);
}

void WDMFeature::HandleSubscriptionEngineEvent(void * appState, SubscriptionEngine::EventID eventType,
                                               const SubscriptionEngine::InEventParam & inParam,
                                               SubscriptionEngine::OutEventParam & outParam)
{
    switch (eventType)
    {
    case SubscriptionEngine::kEvent_OnIncomingSubscribeRequest:
        outParam.mIncomingSubscribeRequest.mHandlerEventCallback = HandleInboundSubscriptionEvent;
        outParam.mIncomingSubscribeRequest.mHandlerAppState      = NULL;
        outParam.mIncomingSubscribeRequest.mRejectRequest        = false;
        break;

    default:
        SubscriptionEngine::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}

bool WDMFeature::AreServiceSubscriptionsEstablished(void)
{
    return (mIsSubToServiceEstablished && mIsServiceCounterSubEstablished);
}

bool WDMFeature::IsOCSensorSubscriptionEstablished(void)
{
    return (mIsSubToOCSensorEstablished);
}

void WDMFeature::InitiateSubscriptionToService(void)
{
    WeaveLogProgress(Support, "Initiating Subscription To Service");
    mServiceSubClient->EnableResubscribe(NULL);
    mServiceSubClient->InitiateSubscription();
}

void WDMFeature::InitiateSubscriptionToOCSensor(void)
{
    WeaveLogProgress(Support, "Initiating Subscription To OCSensor");

    // WdmFeature().TearDownOCSensorSubscription(); // FIXME
    TearDownOCSensorSubscription(); // FIXME
    // Also this method should be static and be called via an app event.
    // WdmFeature().mOCSensorSubClient->EnableResubscribe(NULL);
    mOCSensorSubClient->EnableResubscribe(NULL);
    // WdmFeature().mOCSensorSubClient->InitiateSubscription();
    mOCSensorSubClient->InitiateSubscription();
}

void WDMFeature::TearDownServiceSubscriptions(void)
{
    if (mServiceSubClient)
    {
        mServiceSubClient->AbortSubscription();
        if (mServiceCounterSubHandler != NULL)
        {
            mServiceCounterSubHandler->AbortSubscription();
            mServiceCounterSubHandler = NULL;
        }
    }
}

void WDMFeature::TearDownOCSensorSubscription(void)
{
    if (mOCSensorSubClient)
    {
        mOCSensorSubClient->AbortSubscription();
    }
}

void WDMFeature::HandleServiceBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
                                           const ::nl::Weave::Binding::InEventParam & inParam,
                                           ::nl::Weave::Binding::OutEventParam & outParam)
{
    WeaveLogProgress(Support, "*** HandleServiceBindingEvent: %d", eventType);
    Binding * binding = inParam.Source;

    switch (eventType)
    {
    case Binding::kEvent_PrepareRequested:
        WeaveLogProgress(Support, "*** HandleServiceBindingEvent: PrepareRequested");
        outParam.PrepareRequested.PrepareError = binding->BeginConfiguration()
                                                     .Target_ServiceEndpoint(kServiceEndpoint_Data_Management)
                                                     .Transport_UDP_WRM()
                                                     .Transport_DefaultWRMPConfig(gWRMPConfigService)
                                                     .Exchange_ResponseTimeoutMsec(SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS)
                                                     .Security_SharedCASESession()
                                                     .PrepareBinding();
        break;

    case Binding::kEvent_PrepareFailed:
        WeaveLogProgress(Support, "*** HandleServiceBindingEvent: PrepareFailed");
        WeaveLogError(Support, "Failed to prepare service subscription binding: %s", ErrorStr(inParam.PrepareFailed.Reason));
        break;

    case Binding::kEvent_BindingFailed:
        WeaveLogProgress(Support, "*** HandleServiceBindingEvent: BindingFailed");
        WeaveLogError(Support, "Service subscription binding failed: %s", ErrorStr(inParam.BindingFailed.Reason));
        break;

    case Binding::kEvent_BindingReady:
        WeaveLogProgress(Support, "*** HandleServiceBindingEvent: BindingReady");
        WeaveLogProgress(Support, "Service subscription binding ready");
        break;

    default:
        WeaveLogProgress(Support, "*** HandleServiceBindingEvent: default");
        nl::Weave::Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

// FIXME
void WDMFeature::HandleOCSensorBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
                                            const ::nl::Weave::Binding::InEventParam & inParam,
                                            ::nl::Weave::Binding::OutEventParam & outParam)
{
    Binding * binding = inParam.Source;

    WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent");

    WDMFeature & _this = GetWDMFeature();

    switch (eventType)
    {
    case Binding::kEvent_PrepareRequested:
        WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent:PrepareRequested");
        outParam.PrepareRequested.PrepareError =
            binding->BeginConfiguration()
                .Target_NodeId(_this.mOCSensorNodeId) // fixme: double check
                // FIXME: check with Suryanshu .TargetAddress_WeaveFabric(_this.mOCSensorFabricId)
                .TargetAddress_WeaveFabric(::nl::Weave::kWeaveSubnetId_ThreadMesh)
                .Transport_UDP_WRM()
                .Transport_DefaultWRMPConfig(gWRMPConfigOCSensor)
                .Exchange_ResponseTimeoutMsec(OCSENSOR_MESSAGE_RESPONSE_TIMEOUT_MS)
                .Security_None() // FIXME
                .PrepareBinding();
        break;

    case Binding::kEvent_PrepareFailed:
        WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent:PrepareFailed");
        WeaveLogError(Support, "Failed to prepare OCSensor subscription binding: %s", ErrorStr(inParam.PrepareFailed.Reason));
        break;

    case Binding::kEvent_BindingFailed:
        WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent:BindingFailed");
        WeaveLogError(Support, "OCSensor subscription binding failed: %s", ErrorStr(inParam.BindingFailed.Reason));
        break;

    case Binding::kEvent_BindingReady:
        WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent:BindingReady");
        WeaveLogProgress(Support, "OCSensor subscription binding ready");
        break;

    default:
        WeaveLogProgress(Support, "*** HandleOCSensorBindingEvent:default");
        nl::Weave::Binding::DefaultEventHandler(appState, eventType, inParam, outParam);
    }
}

void WDMFeature::HandleInboundSubscriptionEvent(void * aAppState, SubscriptionHandler::EventID eventType,
                                                const SubscriptionHandler::InEventParam & inParam,
                                                SubscriptionHandler::OutEventParam & outParam)
{
    WeaveLogDetail(Support, "*** WDMFeature::HandleInboundSubscriptionEvent");
    switch (eventType)
    {
    case SubscriptionHandler::kEvent_OnSubscribeRequestParsed: {
        Binding * binding = inParam.mSubscribeRequestParsed.mHandler->GetBinding();

        if (inParam.mSubscribeRequestParsed.mIsSubscriptionIdValid &&
            inParam.mSubscribeRequestParsed.mMsgInfo->SourceNodeId == kServiceEndpoint_Data_Management)
        {
            WeaveLogDetail(Support,
                           "Inbound service counter-subscription request received (sub id %016" PRIX64 ", path count %" PRId16 ")",
                           inParam.mSubscribeRequestParsed.mSubscriptionId, inParam.mSubscribeRequestParsed.mNumTraitInstances);
            sWDMFeature.mServiceCounterSubHandler = inParam.mSubscribeRequestParsed.mHandler;
            binding->SetDefaultResponseTimeout(SERVICE_MESSAGE_RESPONSE_TIMEOUT_MS);
            binding->SetDefaultWRMPConfig(gWRMPConfigService);
        }
        else
        {
            WeaveLogDetail(Support, "Inbound device subscription request received (sub id %016" PRIX64 ", path count %" PRId16 ")",
                           inParam.mSubscribeRequestParsed.mSubscriptionId, inParam.mSubscribeRequestParsed.mNumTraitInstances);
        }

        inParam.mSubscribeRequestParsed.mHandler->AcceptSubscribeRequest(inParam.mSubscribeRequestParsed.mTimeoutSecMin);
        break;
    }

    case SubscriptionHandler::kEvent_OnSubscriptionEstablished: {
        if (inParam.mSubscriptionEstablished.mHandler == sWDMFeature.mServiceCounterSubHandler)
        {
            WeaveLogDetail(Support, "Inbound service counter-subscription established");
            sWDMFeature.mIsServiceCounterSubEstablished = true;
        }
        else
        {
            WeaveLogDetail(Support, "Inbound device subscription established");
        }
        break;
    }

    case SubscriptionHandler::kEvent_OnSubscriptionTerminated: {
        const char * termDesc = (inParam.mSubscriptionTerminated.mReason == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
            ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
            : ErrorStr(inParam.mSubscriptionTerminated.mReason);

        if (inParam.mSubscriptionTerminated.mHandler == sWDMFeature.mServiceCounterSubHandler)
        {
            WeaveLogProgress(Support, "Inbound service counter-subscription terminated: %s", termDesc);

            sWDMFeature.mServiceCounterSubHandler       = NULL;
            sWDMFeature.mIsServiceCounterSubEstablished = false;
        }
        break;
    }

    default:
        SubscriptionHandler::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}

void WDMFeature::HandleOutboundServiceSubscriptionEvent(void * appState, SubscriptionClient::EventID eventType,
                                                        const SubscriptionClient::InEventParam & inParam,
                                                        SubscriptionClient::OutEventParam & outParam)
{
    WeaveLogProgress(Support, "*** HandleOutboundServiceSubscriptionEvent: %d", eventType);
    switch (eventType)
    {
    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded: {
        WeaveLogProgress(Support, "*** HandleOutboundServiceSubscriptionEvent: OnSubscribeRequestPrepareNeeded");
        outParam.mSubscribeRequestPrepareNeeded.mPathList                  = &(sWDMFeature.mServiceSinkTraitPaths[0]);
        outParam.mSubscribeRequestPrepareNeeded.mPathListSize              = kSinkHandle_Max;
        outParam.mSubscribeRequestPrepareNeeded.mVersionedPathList         = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents             = false;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList     = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin             = SERVICE_LIVENESS_TIMEOUT_SEC;
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax             = SERVICE_LIVENESS_TIMEOUT_SEC;

        WeaveLogDetail(Support, "Sending outbound service subscribe request (path count 1)");

        break;
    }
    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogProgress(Support,
                         "*** HandleOutboundServiceSubscriptionEvent: OnSubscriptionEstablished -- Outbound service subscription "
                         "established (sub id %016" PRIX64 ")",
                         inParam.mSubscriptionEstablished.mSubscriptionId);
        sWDMFeature.mIsSubToServiceEstablished = true;
        break;

    case SubscriptionClient::kEvent_OnSubscriptionTerminated: {
        WeaveLogProgress(
            Support, "*** HandleOutboundServiceSubscriptionEvent: OnSubscriptionTerminated -- service subscription terminated: %s",
            (inParam.mSubscriptionTerminated.mIsStatusCodeValid)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason));

        sWDMFeature.mIsSubToServiceEstablished = false;

        if (inParam.mSubscriptionTerminated.mClient == sWDMFeature.mServiceSubClient)
        {
            // This would happen when the service explicitly terminates the subscription
            // by sending a CANCEL subscription request.
            if (!inParam.mSubscriptionTerminated.mWillRetry)
            {
                sWDMFeature.TearDownServiceSubscriptions();
                sWDMFeature.InitiateSubscriptionToService();
            }
        }

        break;
    }

    default:
        WeaveLogProgress(Support, "*** HandleOutboundServiceSubscriptionEvent: default");
        SubscriptionClient::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}

void WDMFeature::HandleOutboundOCSensorSubscriptionEvent(void * appState, SubscriptionClient::EventID eventType,
                                                         const SubscriptionClient::InEventParam & inParam,
                                                         SubscriptionClient::OutEventParam & outParam)
{
    WeaveLogProgress(Support, "*** HandleOutboundOCSensorSubscriptionEvent: %d", eventType);
    switch (eventType)
    {
    case SubscriptionClient::kEvent_OnSubscribeRequestPrepareNeeded: {
        WeaveLogProgress(Support, "*** HandleOutboundOCSensorSubscriptionEvent: OnSubscribeRequestPrepareNeeded");
        outParam.mSubscribeRequestPrepareNeeded.mPathList                  = &(sWDMFeature.mOCSensorSinkTraitPaths[0]);
        outParam.mSubscribeRequestPrepareNeeded.mPathListSize              = kOCSensorSinkHandle_Max;
        outParam.mSubscribeRequestPrepareNeeded.mVersionedPathList         = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mNeedAllEvents             = false;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventList     = NULL;
        outParam.mSubscribeRequestPrepareNeeded.mLastObservedEventListSize = 0;
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMin             = OCSENSOR_LIVENESS_TIMEOUT_SEC;
        outParam.mSubscribeRequestPrepareNeeded.mTimeoutSecMax             = OCSENSOR_LIVENESS_TIMEOUT_SEC;

        WeaveLogDetail(Support, "Sending outbound OCSensor subscribe request (path count 1)");

        break;
    }
    case SubscriptionClient::kEvent_OnSubscriptionEstablished:
        WeaveLogProgress(Support, "*** HandleOutboundOCSensorSubscriptionEvent: OnSubscriptionEstablished");
        WeaveLogDetail(Support, "Outbound OCSensor subscription established (sub id %016" PRIX64 ")",
                       inParam.mSubscriptionEstablished.mSubscriptionId);
        sWDMFeature.mIsSubToOCSensorEstablished = true;
        // fixme: evaluatechange
        break;

    case SubscriptionClient::kEvent_OnSubscriptionTerminated: {
        WeaveLogProgress(Support, "*** HandleOutboundOCSensorSubscriptionEvent: OnSubscriptionTerminated");
        WeaveLogDetail(
            Support, "Outbound OCSensor subscription terminated: %s",
            (inParam.mSubscriptionTerminated.mIsStatusCodeValid)
                ? StatusReportStr(inParam.mSubscriptionTerminated.mStatusProfileId, inParam.mSubscriptionTerminated.mStatusCode)
                : ErrorStr(inParam.mSubscriptionTerminated.mReason));

        sWDMFeature.mIsSubToOCSensorEstablished = false;
        // fixme: evaluatechange

        if (inParam.mSubscriptionTerminated.mClient == sWDMFeature.mOCSensorSubClient)
        {
            // This would happen when the service explicitly terminates the subscription
            // by sending a CANCEL subscription request.
            if (!inParam.mSubscriptionTerminated.mWillRetry)
            {
                sWDMFeature.TearDownOCSensorSubscription();
                sWDMFeature.InitiateSubscriptionToOCSensor();
            }
        }

        break;
    }

    default:
        WeaveLogProgress(Support, "*** HandleOutboundOCSensorSubscriptionEvent: default");
        SubscriptionClient::DefaultEventHandler(eventType, inParam, outParam);
        break;
    }
}

// FIXME: no clue what is going on here. The WeaveDeviceEvent is not even taken into consideration.
// What are we supposed to do for OCSensor?
void WDMFeature::PlatformEventHandler(const WeaveDeviceEvent * event, intptr_t arg)
{
    WeaveLogProgress(Support, "*** WDMFeature::PlatformEventHandler **** What exactly is happening here???");

    bool serviceSubShouldBeActivated = (ConnectivityMgr().HaveServiceConnectivity() && ConfigurationMgr().IsPairedToAccount());
    WeaveLogProgress(Support, "*** WDMFeature::PlatformEventHandler serviceSubShouldBeActivated:%d mIsSubToServiceActivated:%d",
                     serviceSubShouldBeActivated, sWDMFeature.mIsSubToServiceActivated);

    // If we should be activated and we are not, initiate subscription
    if (serviceSubShouldBeActivated == true && sWDMFeature.mIsSubToServiceActivated == false)
    {
        sWDMFeature.InitiateSubscriptionToService();
        sWDMFeature.mIsSubToServiceActivated = true;
    }
    // If we should be activated and we already are, but not established and not in progress
    else if (serviceSubShouldBeActivated && sWDMFeature.mIsSubToServiceActivated && !sWDMFeature.mIsSubToServiceEstablished &&
             !sWDMFeature.mServiceSubClient->IsInProgressOrEstablished())
    {
        // Connectivity might have just come back again. Reset Resubscribe Interval
        sWDMFeature.mServiceSubClient->ResetResubscribe();
    }
}

/**
 * Invoked once the association between Lock and OCSensor has been established.
 */
WEAVE_ERROR WDMFeature::SetupOCSensorSubscriptions(uint64_t nodeId, uint64_t fabricId)
{
    WEAVE_ERROR err;

    WeaveLogDetail(Support, "*WDMFeature::SetupOCSensorSubscriptions");

    mOCSensorNodeId   = nodeId;
    mOCSensorFabricId = fabricId;

    mOCSensorSubBinding = ExchangeMgr.NewBinding(HandleOCSensorBindingEvent, this);
    VerifyOrExit(NULL != mOCSensorSubBinding, err = WEAVE_ERROR_NO_MEMORY);
    err = mSubscriptionEngine.NewClient(&(mOCSensorSubClient), mOCSensorSubBinding, this, HandleOutboundOCSensorSubscriptionEvent,
                                        &mOCSensorSinkTraitCatalog, SUBSCRIPTION_RESPONSE_TIMEOUT_MS);
    SuccessOrExit(err);

    InitiateSubscriptionToOCSensor();

exit:
    return err;
}

WEAVE_ERROR WDMFeature::Init()
{
    WEAVE_ERROR err;
    int ret;

    // FIXME: Remove the use of NRF_ERROR_NULL
    // Once this is fixed, we can remove the nrf5 dependency. Could we use this?
    //    err = mPublisherLock.Init();
    //    SuccessOrExit(err);
    ret = mPublisherLock.Init();
    VerifyOrExit(ret != NRF_ERROR_NULL, err = WEAVE_ERROR_NO_MEMORY);

    PlatformMgr().AddEventHandler(PlatformEventHandler);

    mServiceSourceTraitCatalog.AddAt(0, &mBoltLockTraitSource, kSourceHandle_BoltLockTrait);
    mServiceSourceTraitCatalog.AddAt(0, &mDeviceIdentityTraitSource, kSourceHandle_DeviceIdentityTrait);

    mServiceSinkTraitCatalog.AddAt(0, &mBoltLockSettingsTraitSink, kSinkHandle_BoltLockSettingsTrait);
    for (uint8_t handle = 0; handle < kSinkHandle_Max; handle++)
    {
        mServiceSinkTraitPaths[handle].mTraitDataHandle    = handle;
        mServiceSinkTraitPaths[handle].mPropertyPathHandle = kRootPropertyPathHandle;
    }

    mOCSensorSinkTraitCatalog.AddAt(0, &mOCSensorSecurityOpenCloseTraitSink, kOCSensorSinkHandle_SecurityOpenCloseTrait);
    for (uint8_t handle = 0; handle < kOCSensorSinkHandle_Max; handle++)
    {
        mOCSensorSinkTraitPaths[handle].mTraitDataHandle    = handle;
        mOCSensorSinkTraitPaths[handle].mPropertyPathHandle = kRootPropertyPathHandle;
    }

    err = mSubscriptionEngine.Init(&ExchangeMgr, this, HandleSubscriptionEngineEvent);
    SuccessOrExit(err);

    err = mSubscriptionEngine.EnablePublisher(&mPublisherLock, &mServiceSourceTraitCatalog);
    SuccessOrExit(err);

    // Bind sinks
    mServiceSubBinding = ExchangeMgr.NewBinding(HandleServiceBindingEvent, this);
    VerifyOrExit(NULL != mServiceSubBinding, err = WEAVE_ERROR_NO_MEMORY);
    err = mSubscriptionEngine.NewClient(&(mServiceSubClient), mServiceSubBinding, this, HandleOutboundServiceSubscriptionEvent,
                                        &mServiceSinkTraitCatalog, SUBSCRIPTION_RESPONSE_TIMEOUT_MS);
    SuccessOrExit(err);

    WeaveLogProgress(Support, "WDMFeature Init Complete");

exit:
    return err;
}