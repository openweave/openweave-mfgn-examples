/*
 *
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

#include "DeviceController.h"
#include "HardwarePlatform.h"
#include "AppTask.h"

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/DeviceLayer/SoftwareUpdateManager.h>
#include <Weave/DeviceLayer/ConfigurationManager.h>

#include "app_timer.h"
#include "FreeRTOS.h"

#include "BoltLockTrait.h"
#include "AppSoftwareUpdateManager.h"
#include "ConnectivityState.h"
#include "WDMFeature.h"
#include "AppTask.h"

using namespace ::nl::Weave::DeviceLayer;

extern void SuccessOrAbort(WEAVE_ERROR ret, const char * msg);

// -----------------------------------------------------------------------------
// Initializations

// Singleton object.
DeviceController DeviceController::sDeviceController;

// The Device Timer is a FreeRTOS timer used for the following purposes:
//   - To simulate bolt movement of the lock device. The timer is set for the
//     time it takes for the bolt to be fully extended or retracted.
//     (ACTUATOR_MOVEMENT_CONTEXT)
//   - To support auto-lock of the lock device. When the bolt is unlocked,
//     the timer is set to the period of time after which we want it to be
//     automatically locked. (AUTOLOCK_CONTEXT)
APP_TIMER_DEF(sDeviceTimer);

// -----------------------------------------------------------------------------
// DeviceManager API (see common/include/DeviceManager.h)

void DeviceController::Init()
{
    WEAVE_ERROR ret;

    // Create the device's timer.
    ret = app_timer_create(&sDeviceTimer, APP_TIMER_MODE_SINGLE_SHOT, DeviceTimerEventHandler);
    SuccessOrAbort(ret, "app_timer_create failed.");

    // Initial state of the lock.
    mState                        = kState_LockingCompleted;
    mLongPressButtonEventInFlight = false;
    mAutoLockTimerArmed           = false;
    mAutoLockEnabled              = false;
    mAutoLockDurationSeconds      = 0;

    WeaveLogProgress(Support, "Initializing the Buttons");
    Button * buttons = GetHardwarePlatform().GetButtons();
    (buttons + BUTTON_1_INDEX)->SetShortPressEventHandler(SoftwareUpdateButtonHandler);
    (buttons + BUTTON_1_INDEX)->SetLongPressEventHandler(FactoryResetButtonHandler);
    (buttons + BUTTON_2_INDEX)->SetShortPressEventHandler(LockButtonEventHandler);
    (buttons + BUTTON_2_INDEX)->SetLongPressEventHandler(SendIdentifyRequestButtonHandler);

    WeaveLogProgress(Support, "Initializing the LEDs");
    LED * leds               = GetHardwarePlatform().GetLEDs();
    mConnectivityStateLEDPtr = leds + CONNECTIVITY_STATE_LED_INDEX;
    mLockStateLEDPtr         = leds + LOCK_STATE_LED_INDEX;
    mLockStateLEDPtr->Set(!IsUnlocked());

    // FIXME: How do I ensure that the state of the lock in the service is in sync with
    // the current state of the physical device?
    WeaveLogProgress(Support, "Initializing WDMFeature");
    ret = GetWDMFeature().Init();
    SuccessOrAbort(ret, "GetWDMFeature().Init() failed.");

    // Initialize the Manager for Software Updates (SWU aka OTA).
    GetAppSoftwareUpdateManager().Init();

    // Setup the ConnectivityState object that reflects the provisioning state on a LED.
    mConnectivityState.SetLED(mConnectivityStateLEDPtr);

    // Setup the DeviceDescription client.
    WeaveLogProgress(Support, "Initializing DeviceDescriptionClient");
    ret = mDeviceDescriptionClient.Init(&ExchangeMgr);
    SuccessOrAbort(ret, "DeviceDescriptionClient.Init() failed.");
    mDeviceDescriptionClient.OnIdentifyResponseReceived = OnIdentifyResponseReceivedHandler;

    // Print the current software version.
    char currentFirmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength + 1] = { 0 };
    size_t currentFirmwareRevLen;
    ret = ConfigurationMgr().GetFirmwareRevision(currentFirmwareRev, sizeof(currentFirmwareRev), currentFirmwareRevLen);
    SuccessOrAbort(ret, "ConfigurationMgr().GetFirmwareRevision() failed.");
    WeaveLogProgress(Support, "Current Firmware Version: %s", currentFirmwareRev);
}

void DeviceController::EventLoopCycle()
{
    DeviceController & _this = GetDeviceController();
    LED * leds               = GetHardwarePlatform().GetLEDs();
    Button * buttons         = GetHardwarePlatform().GetButtons();

    // Update the button press state.
    // If LongPressInFlight state changes, then update LEDs state accordingly
    // (All LEDs blink when in "LongPressInFlight".)
    bool startLongPressInFlight = false;
    bool endLongPressInFlight   = false;
    for (int idx = 0; idx < PLATFORM_BUTTONS_COUNT; idx++)
    {
        Button::ButtonPressState buttonPressState = (buttons + idx)->UpdateButtonPressState();
        if (buttonPressState == Button::kButtonPressState_Long_Started)
        {
            if (!_this.mLongPressButtonEventInFlight)
            {
                _this.mLongPressButtonEventInFlight = true;
                startLongPressInFlight              = true;
            }
        }
        else if (buttonPressState == Button::kButtonPressState_Long_Completed)
        {
            if (_this.mLongPressButtonEventInFlight)
            {
                _this.mLongPressButtonEventInFlight = false;
                endLongPressInFlight                = true;
            }
        }
    }

    if (startLongPressInFlight)
    {
        // Indicate that long press is in flight on the LEDs by blinking all of them.
        // Turn off all LEDs before starting blink to make sure blink is co-ordinated.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Set(false);
        }
        // Blink all LEDs.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Blink(500);
        }
    }
    else if (endLongPressInFlight)
    {
        // Reset all the LEDs.
        for (int led_idx = 0; led_idx < PLATFORM_LEDS_COUNT; led_idx++)
        {
            (leds + led_idx)->Set(false);
        }

        // Set lock status LED back to show state of lock.
        _this.mLockStateLEDPtr->Set(!_this.IsUnlocked());

        // Update the provisioning state shown on LED.
        _this.mConnectivityState.Update(GetWDMFeature().AreServiceSubscriptionsEstablished());
    }
    else if (!_this.mLongPressButtonEventInFlight)
    {
        // Update the provisioning state shown on LED.
        _this.mConnectivityState.Update(GetWDMFeature().AreServiceSubscriptionsEstablished());
    }

    // Animate the LEDs.
    for (int idx = 0; idx < PLATFORM_LEDS_COUNT; idx++)
    {
        (leds + idx)->Animate();
    }
}

// -----------------------------------------------------------------------------
// Accessors

bool DeviceController::IsLockingActionInProgress()
{
    return (mState == kState_LockingInitiated || mState == kState_UnlockingInitiated);
}

bool DeviceController::IsUnlocked()
{
    return (mState == kState_UnlockingCompleted);
}

void DeviceController::EnableAutoLock(bool aOn)
{
    mAutoLockEnabled = aOn;
}

void DeviceController::SetAutoLockDuration(uint32_t aDurationSeconds)
{
    mAutoLockDurationSeconds = aDurationSeconds;
}

// -----------------------------------------------------------------------------
// Timer Management

void DeviceController::StartTimer(uint32_t aTimeoutMs)
{
    ret_code_t ret;
    ret = app_timer_start(sDeviceTimer, pdMS_TO_TICKS(aTimeoutMs), NULL);
    SuccessOrAbort(ret, "app_timer_start() failed");
}

void DeviceController::CancelTimer(void)
{
    ret_code_t ret;
    ret = app_timer_stop(sDeviceTimer);
    SuccessOrAbort(ret, "app_timer_stop() failed");
}

void DeviceController::DeviceTimerEventHandler(void * p_context)
{
    DeviceController & _this = GetDeviceController();
    TimerContext_t context   = _this.mTimerContext;

    // The timer event handler is called in the context of the timer task
    // once sLockTimer expires.
    // Posts an event to AppTask queue with the proper handler. The event will then be handled
    // in the context of the application task.
    AppTask::AppTaskEventHandler_t eventHandler;
    switch (context)
    {
    case AUTO_LOCK_CONTEXT:
        eventHandler = AutoLockTimerEventHandler;
        break;
    case ACTUATOR_MOVEMENT_CONTEXT:
        eventHandler = ActuatorMovementTimerEventHandler;
        break;
    default:
        WeaveLogError(Support, "ERROR: Invalid context for device timer: %d", context);
    }
    AppTask::AppTaskEvent appTaskEvent;
    appTaskEvent.Handler = eventHandler;
    appTaskEvent.Data    = nullptr;
    GetAppTask().PostEvent(&appTaskEvent);
}

void DeviceController::AutoLockTimerEventHandler(void * data)
{
    DeviceController & _this = GetDeviceController();

    // Make sure auto lock timer is still armed.
    if (!_this.mAutoLockTimerArmed)
    {
        return;
    }

    WeaveLogDetail(Support, "Auto-Lock has been triggered!");
    int32_t actor             = Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_LOCAL_IMPLICIT;
    _this.mAutoLockTimerArmed = false;
    _this.InitiateAction(actor, LOCK_ACTION);
}

void DeviceController::ActuatorMovementTimerEventHandler(void * data)
{
    DeviceController & _this = GetDeviceController();

    if (_this.mState == kState_LockingInitiated)
    {
        _this.mState = kState_LockingCompleted;
        _this.ActionCompleted(LOCK_ACTION);
    }
    else if (_this.mState == kState_UnlockingInitiated)
    {
        _this.mState = kState_UnlockingCompleted;
        _this.ActionCompleted(UNLOCK_ACTION);
    }
}

bool DeviceController::InitiateAction(int32_t aActor, Action_t aAction)
{
    // We can initiate a Lock/Unlock Action only if the previous one has completed
    bool action_initiated = false;
    State_t new_state;

    if (aAction == UNLOCK_ACTION && mState == kState_LockingCompleted)
    {
        // Unlock and locking is completed, all good to move to unlocking state.
        action_initiated = true;
        new_state        = kState_UnlockingInitiated;
    }
    else if (aAction == LOCK_ACTION && mState == kState_UnlockingCompleted)
    {
        // Lock and unlocking is completed, all good to move to locking state.
        action_initiated = true;
        new_state        = kState_LockingInitiated;
    }

    WeaveLogDetail(Support, "action_initiated [%d] mAutoLockTimerArmed [%d] new_state: [%d]", action_initiated, mAutoLockTimerArmed,
                   new_state);
    if (action_initiated)
    {
        if (mAutoLockTimerArmed && new_state == kState_LockingInitiated)
        {
            // Auto lock timer has been armed and someone initiates locking,
            // cancel the timer and continue as normal.
            mAutoLockTimerArmed = false;
            CancelTimer();
        }

        // Simulate the bolt movement for a period of time. Timer is started.
        mTimerContext = ACTUATOR_MOVEMENT_CONTEXT;
        StartTimer(ACTUATOR_MOVEMENT_DURATION_MS);

        // Since the timer started successfully, update the state and trigger callback
        mState = new_state;

        ActionInitiated(aAction, aActor);
    }

    return action_initiated;
}

void DeviceController::ActionInitiated(DeviceController::Action_t aAction, int32_t aActor)
{
    // If the action has been initiated by the lock, update the bolt lock trait
    // and start flashing the LEDs rapidly to indicate action initiation.
    if (aAction == DeviceController::LOCK_ACTION)
    {
        GetWDMFeature().GetBoltLockTraitDataSource().InitiateLock(aActor);
        WeaveLogDetail(Support, "Lock Action has been initiated");
    }
    else if (aAction == DeviceController::UNLOCK_ACTION)
    {
        GetWDMFeature().GetBoltLockTraitDataSource().InitiateUnlock(aActor);
        WeaveLogDetail(Support, "Unlock Action has been initiated");
    }

    WeaveLogDetail(Support, "blinking the LockState LED");
    mLockStateLEDPtr->Blink(50, 50);
}

void DeviceController::ActionCompleted(DeviceController::Action_t aAction)
{
    // if the action has been completed by the lock, update the bolt lock trait.
    // Turn on the lock LED if in a LOCKED state OR
    // Turn off the lock LED if in an UNLOCKED state.
    if (aAction == DeviceController::LOCK_ACTION)
    {
        WeaveLogDetail(Support, "Lock Action has been completed");

        GetWDMFeature().GetBoltLockTraitDataSource().LockingSuccessful();
        mLockStateLEDPtr->Set(true);
    }
    else if (aAction == DeviceController::UNLOCK_ACTION)
    {
        WeaveLogDetail(Support, "Unlock Action has been completed");
        GetWDMFeature().GetBoltLockTraitDataSource().UnlockingSuccessful();
        mLockStateLEDPtr->Set(false);

        if (mAutoLockEnabled)
        {
            // Start the timer for auto-lock
            mTimerContext       = AUTO_LOCK_CONTEXT;
            mAutoLockTimerArmed = true;
            StartTimer(mAutoLockDurationSeconds * 1000);
            WeaveLogDetail(Support, "Auto-lock enabled. Will be triggered in %u seconds", mAutoLockDurationSeconds);
        }
    }
}

// -----------------------------------------------------------------------------
// Event Handlers

void DeviceController::LockButtonEventHandler()
{
    WeaveLogDetail(Support, "DeviceController::LockButtonEventHandler");
    DeviceController & _this = GetDeviceController();

    Action_t action;
    int32_t actor;

    if (_this.IsUnlocked())
    {
        // Bolt is currently unlocked -> now we need to lock it.
        action = LOCK_ACTION;
    }
    else
    {
        // Bolt is currently locked -> now we need to unlock it.
        action = UNLOCK_ACTION;
    }
    actor = Schema::Weave::Trait::Security::BoltLockTrait::BOLT_LOCK_ACTOR_METHOD_PHYSICAL;

    bool initiated = _this.InitiateAction(actor, action);
    if (!initiated)
    {
        // We are already in the process of locking/unlocking the bolt.
        WeaveLogDetail(Support, "Action is already in progress or active.");
    }
}

void DeviceController::LockOnCommandRequestEventHandler(void * eventData)
{
    WeaveLogDetail(Support, "DeviceController::LockOnCommandRequestEventHandler");

    DeviceController & _this = GetDeviceController();

    LockOnCommandRequestData * data = static_cast<LockOnCommandRequestData *>(eventData);
    bool initiated                  = _this.InitiateAction(data->actor, data->action);
    if (!initiated)
    {
        // We are already in the process of locking/unlocking the bolt.
        WeaveLogDetail(Support, "Action is already in progress or active.");
    }
}

void DeviceController::SoftwareUpdateButtonHandler()
{
    WeaveLogDetail(Support, "Manual Software Update Triggered");
    GetAppSoftwareUpdateManager().CheckNow();
}

void DeviceController::FactoryResetButtonHandler()
{
    WeaveLogDetail(Support, "Factory Reset Triggered.");
    nl::Weave::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
}

void DeviceController::SendIdentifyRequestButtonHandler()
{
    WeaveLogProgress(Support, "DeviceController::SendIdentifyRequestButtonHandler()");

    DeviceController & _this = GetDeviceController();
    WEAVE_ERROR err          = WEAVE_NO_ERROR;
    IdentifyRequestMessage identifyReqMsg;
    nl::Inet::IPAddress ip_addr;

    if (!ConfigurationMgr().IsMemberOfFabric())
    {
        WeaveLogError(Support, "DeviceDiscovery err: Device not fabric provisioned");
        return;
    }
    uint16_t vendorId;
    ConfigurationMgr().GetVendorId(vendorId);
    uint16_t productId;
    ConfigurationMgr().GetProductId(productId);

    // FIXME: Because these are Sleepy End Devices, mut set the multicast address in a special way.
    // Waiting for Tread team to provide that information.
    ip_addr                        = nl::Inet::IPAddress::MakeIPv6WellKnownMulticast(nl::Inet::kIPv6MulticastScope_Realm,
                                                              nl::Inet::kIPV6MulticastGroup_AllNodes);
    identifyReqMsg.TargetFabricId  = ::nl::Weave::DeviceLayer::FabricState.FabricId;
    identifyReqMsg.TargetModes     = kTargetDeviceMode_UserSelectedMode;
    identifyReqMsg.TargetVendorId  = vendorId;
    identifyReqMsg.TargetProductId = productId;
    identifyReqMsg.TargetDeviceId  = nl::Weave::kAnyNodeId;

    WeaveLogProgress(Support, "Sending the Identify request");
    err = _this.mDeviceDescriptionClient.SendIdentifyRequest(ip_addr, identifyReqMsg);
    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(Support, "SendIdentifyRequest failed: [%d]", err);
        return;
    }
}

void DeviceController::OnIdentifyResponseReceivedHandler(void * appState, uint64_t nodeId, const IPAddress & nodeAddr,
                                                         const IdentifyResponseMessage & respMsg)
{
    WeaveLogProgress(Support, "OnIdentifyResponseReceivedHandler");
    DeviceController & _this         = GetDeviceController();
    WeaveDeviceDescriptor deviceDesc = respMsg.DeviceDesc;
    char ipAddrStr[64];
    nodeAddr.ToString(ipAddrStr, sizeof(ipAddrStr));
    WeaveLogDetail(Support, "IdentifyResponse received from node %" PRIX64 " (%s)\n", nodeId, ipAddrStr);
    WeaveLogDetail(Support, "  Source Fabric Id: %016" PRIX64 "\n", deviceDesc.FabricId);
    WeaveLogDetail(Support, "  Source Vendor Id: %04X\n", (unsigned) deviceDesc.VendorId);
    WeaveLogDetail(Support, "  Source Product Id: %04X\n", (unsigned) deviceDesc.ProductId);
    WeaveLogDetail(Support, "  Source Product Revision: %04X\n", (unsigned) deviceDesc.ProductRevision);
    _this.mDeviceDescriptionClient.CancelExchange();

    // FIXME: post an event to record the info...
}

// -----------------------------------------------------------------------------
// Utility Methods

// This is called by BoltLockTraitDataSource::OnCustomCommand.
void DeviceController::PostLockOnCommandRequestEvent(int32_t actor, Action_t action)
{
    LockOnCommandRequestData data;
    data.actor  = actor;
    data.action = action;

    AppTask::AppTaskEvent appTaskEvent;
    appTaskEvent.Handler = LockOnCommandRequestEventHandler;
    appTaskEvent.Data    = &data;
    GetAppTask().PostEvent(&appTaskEvent);
}
