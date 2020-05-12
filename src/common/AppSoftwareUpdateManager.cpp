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

#include "AppSoftwareUpdateManager.h"

#include "AppTask.h"

#include <Weave/DeviceLayer/SoftwareUpdateManager.h>
#include <Weave/Support/crypto/HashAlgos.h>

static nl::Weave::Platform::Security::SHA256 sSHA256;

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::DeviceLayer;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;

// Singleton.
AppSoftwareUpdateManager AppSoftwareUpdateManager::sAppSoftwareUpdateManager;

#ifndef APP_ERROR_CHECK
    #define APP_ERROR_CHECK(ERR_CODE)                       \
    do                                                      \
    {                                                       \
        const uint32_t LOCAL_ERR_CODE = (ERR_CODE);         \
        if (LOCAL_ERR_CODE != 0)                            \
        {                                                   \
            return;                                         \
        }                                                   \
    } while (0)
#endif

void AppSoftwareUpdateManager::Init(void)
{
    // WEAVE_ERROR SetEventCallback(void * const aAppState, const EventCallback aEventCallback);
    SoftwareUpdateMgr().SetEventCallback(NULL, HandleSoftwareUpdateEvent);

    // Enable timer based Software Update Checks
    SoftwareUpdateMgr().SetQueryIntervalWindow(SWU_INTERVAl_WINDOW_MIN_MS, SWU_INTERVAl_WINDOW_MAX_MS);
}

bool AppSoftwareUpdateManager::IsInProgress(void)
{
    return SoftwareUpdateMgr().IsInProgress();
}

void AppSoftwareUpdateManager::Abort(void)
{
    SoftwareUpdateMgr().Abort();
}

void AppSoftwareUpdateManager::CheckNow(void)
{
    SoftwareUpdateMgr().CheckNow();
}

void AppSoftwareUpdateManager::InstallEventHandler(void * data)
{
    SoftwareUpdateMgr().ImageInstallComplete(WEAVE_NO_ERROR);
}

void AppSoftwareUpdateManager::HandleSoftwareUpdateEvent(void * apAppState, SoftwareUpdateManager::EventType aEvent,
                                                         const SoftwareUpdateManager::InEventParam & aInParam,
                                                         SoftwareUpdateManager::OutEventParam & aOutParam)
{
    static uint32_t persistedImageLen                                              = 0;
    static char persistedImageURI[WEAVE_DEVICE_CONFIG_SOFTWARE_UPDATE_URI_LEN + 1] = "";

    switch (aEvent)
    {
    case SoftwareUpdateManager::kEvent_PrepareQuery: {
        aOutParam.PrepareQuery.PackageSpecification = NULL;
        aOutParam.PrepareQuery.DesiredLocale        = NULL;
        break;
    }

    case SoftwareUpdateManager::kEvent_PrepareQuery_Metadata: {
        WEAVE_ERROR err;
        bool haveSufficientBattery = true;
        uint32_t certBodyId        = 0;

        TLVWriter * writer = aInParam.PrepareQuery_Metadata.MetaDataWriter;

        if (writer)
        {
            // Providing an installed Locale as MetaData is optional. The commented section below provides an example
            // of how one can be added to metadata.

            // TLVType arrayContainerType;
            // err = writer->StartContainer(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_InstalledLocales),
            // kTLVType_Array, arrayContainerType); SuccessOrExit(err); err = writer->PutString(AnonymousTag, installedLocale);
            // SuccessOrExit(err);
            // err = writer->EndContainer(arrayContainerType);

            err = writer->Put(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_CertBodyId), certBodyId);
            APP_ERROR_CHECK(err);

            err = writer->PutBoolean(ProfileTag(::nl::Weave::Profiles::kWeaveProfile_SWU, kTag_SufficientBatterySWU),
                                     haveSufficientBattery);
            APP_ERROR_CHECK(err);
        }
        else
        {
            aOutParam.PrepareQuery_Metadata.Error = WEAVE_ERROR_INVALID_ARGUMENT;
            WeaveLogError(Support, "ERROR ! aOutParam.PrepareQuery_Metadata.MetaDataWriter is NULL");
        }
        break;
    }

    case SoftwareUpdateManager::kEvent_QueryPrepareFailed: {
        if (aInParam.QueryPrepareFailed.Error == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
        {
            WeaveLogError(Support, "Software Update failed during prepare: Received StatusReport %s",
                          nl::StatusReportStr(aInParam.QueryPrepareFailed.StatusReport->mProfileId,
                                              aInParam.QueryPrepareFailed.StatusReport->mStatusCode));
        }
        else
        {
            WeaveLogError(Support, "Software Update failed during prepare: %s", nl::ErrorStr(aInParam.QueryPrepareFailed.Error));
        }
        break;
    }

    case SoftwareUpdateManager::kEvent_SoftwareUpdateAvailable: {
        WEAVE_ERROR err;

        char currentFirmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength + 1] = { 0 };
        size_t currentFirmwareRevLen;
        err = ConfigurationMgr().GetFirmwareRevision(currentFirmwareRev, sizeof(currentFirmwareRev), currentFirmwareRevLen);
        APP_ERROR_CHECK(err);

        WeaveLogDetail(Support, "Current Firmware Version: %s", currentFirmwareRev);

        WeaveLogDetail(Support, "Software Update Available - Priority: %d Condition: %d Version: %s IntegrityType: %d URI: %s",
                       aInParam.SoftwareUpdateAvailable.Priority, aInParam.SoftwareUpdateAvailable.Condition,
                       aInParam.SoftwareUpdateAvailable.Version, aInParam.SoftwareUpdateAvailable.IntegrityType,
                       aInParam.SoftwareUpdateAvailable.URI);

        break;
    }

    case SoftwareUpdateManager::kEvent_FetchPartialImageInfo: {
        WeaveLogProgress(Support, "Fetching Partial Image Information");
        if (strcmp(aInParam.FetchPartialImageInfo.URI, persistedImageURI) == 0)
        {
            WeaveLogDetail(Support, "Partial image detected in local storage; resuming download at offset %" PRId32,
                           persistedImageLen);
            aOutParam.FetchPartialImageInfo.PartialImageLen = persistedImageLen;
        }
        else
        {
            WeaveLogDetail(Support, "No partial image detected in local storage");
            aOutParam.FetchPartialImageInfo.PartialImageLen = 0;
        }
        break;
    }

    case SoftwareUpdateManager::kEvent_PrepareImageStorage: {
        WeaveLogProgress(Support, "Preparing Image Storage");

        // Capture state information about the image being downloaded.
        persistedImageLen = 0;
        strncpy(persistedImageURI, aInParam.PrepareImageStorage.URI, sizeof(persistedImageURI));
        persistedImageURI[sizeof(persistedImageURI) - 1] = 0;

        // Prepare to compute the integrity of the image as it is received.
        //
        // This example does not actually store image blocks in persistent storage and merely discards
        // them after computing the SHA over it. As a result, integrity has to be computed as the image
        // blocks are received, rather than over the entire image at the end. This pattern is NOT
        // recommended since the computed integrity will be lost if the device rebooted during download.
        //
        sSHA256.Begin();

        // Tell the SoftwareUpdateManager that storage preparation has completed.
        SoftwareUpdateMgr().PrepareImageStorageComplete(WEAVE_NO_ERROR);
        break;
    }

    case SoftwareUpdateManager::kEvent_StartImageDownload: {
        WeaveLogProgress(Support, "Starting Image Download");
        break;
    }
    case SoftwareUpdateManager::kEvent_StoreImageBlock: {
        sSHA256.AddData(aInParam.StoreImageBlock.DataBlock, aInParam.StoreImageBlock.DataBlockLen);
        persistedImageLen += aInParam.StoreImageBlock.DataBlockLen;
        WeaveLogDetail(Support, "Image Download: %" PRId32 " bytes received", persistedImageLen);
        break;
    }

    case SoftwareUpdateManager::kEvent_ComputeImageIntegrity: {
        WeaveLogProgress(Support, "Computing image integrity");
        WeaveLogDetail(Support, "Total image length: %" PRId32, persistedImageLen);

        // Make sure that the buffer provided in the parameter is large enough.
        if (aInParam.ComputeImageIntegrity.IntegrityValueBufLen < sSHA256.kHashLength)
        {
            aOutParam.ComputeImageIntegrity.Error = WEAVE_ERROR_BUFFER_TOO_SMALL;
        }
        else
        {
            sSHA256.Finish(aInParam.ComputeImageIntegrity.IntegrityValueBuf);
        }
        break;
    }

    case SoftwareUpdateManager::kEvent_ResetPartialImageInfo: {
        // Reset the "persistent" state information related to the image being downloaded,
        // This ensures that the image will be re-downloaded in its entirety during the next
        // software update attempt.
        persistedImageLen    = 0;
        persistedImageURI[0] = '\0';
        break;
    }

    case SoftwareUpdateManager::kEvent_ReadyToInstall: {
        WeaveLogProgress(Support, "Image is ready to be installed");
        break;
    }

    case SoftwareUpdateManager::kEvent_StartInstallImage: {
        // Need to post event
        // AppTask::PostEvent
        //            AppTask* _this = static_cast<AppTask*>(apAppState);
        //            AppTask appTask = GetAppTask();

        WeaveLogProgress(Support, "Image Install is not supported in this example application");

        // FIXME: Which task are we on when this executes? Weave task?
        // Do we really have to post an event to the AppTask?
        AppTask::AppTaskEvent event;
        event.Handler = InstallEventHandler;
        event.Data    = nullptr;
        GetAppTask().PostEvent(&event);

        break;
    }

    case SoftwareUpdateManager::kEvent_Finished: {
        if (aInParam.Finished.Error == WEAVE_ERROR_NO_SW_UPDATE_AVAILABLE)
        {
            WeaveLogProgress(Support, "No Software Update Available");
        }
        else if (aInParam.Finished.Error == WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_IGNORED)
        {
            WeaveLogProgress(Support, "Software Update Ignored by Application");
        }
        else if (aInParam.Finished.Error == WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_ABORTED)
        {
            WeaveLogProgress(Support, "Software Update Aborted by Application");
        }
        else if (aInParam.Finished.Error != WEAVE_NO_ERROR || aInParam.Finished.StatusReport != NULL)
        {
            if (aInParam.Finished.Error == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
            {
                WeaveLogError(
                    Support, "Software Update failed: Received StatusReport %s",
                    nl::StatusReportStr(aInParam.Finished.StatusReport->mProfileId, aInParam.Finished.StatusReport->mStatusCode));
            }
            else
            {
                WeaveLogError(Support, "Software Update failed: %s", nl::ErrorStr(aInParam.Finished.Error));
            }
        }
        else
        {
            WeaveLogProgress(Support, "Software Update Completed");

            // Reset the "persistent" image state information.  Since we don't actually apply the
            // downloaded image, this ensures that the next software update attempt will re-download
            // the image.
            persistedImageLen    = 0;
            persistedImageURI[0] = '\0';
        }
        break;
    }

    default:
        nl::Weave::DeviceLayer::SoftwareUpdateManager::DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
        break;
    }
}