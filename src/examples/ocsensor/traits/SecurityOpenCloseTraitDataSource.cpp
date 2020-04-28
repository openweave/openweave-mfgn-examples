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

/**
 *    @file
 *      A trait data source implementation for the SecurityOpenCloseTrait.
 */

#include "SecurityOpenCloseTraitDataSource.h"

#include "app_config.h"

#include <nest/trait/detector/OpenCloseTrait.h>
#include <nest/trait/security/SecurityOpenCloseTrait.h>
#include <WDMFeature.h>
#include <AppTask.h>

#include <Weave/DeviceLayer/WeaveDeviceLayer.h>
#include <Weave/Support/TraitEventUtils.h>

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;
using namespace Schema::Nest::Trait::Security;
using namespace Schema::Nest::Trait::Detector::OpenCloseTrait;
using namespace Schema::Nest::Trait::Security::SecurityOpenCloseTrait;

SecurityOpenCloseTraitDataSource::SecurityOpenCloseTraitDataSource() : TraitDataSource(&SecurityOpenCloseTrait::TraitSchema)
{
    // FIXME: test this: device is paired, status is open at the service.
    // reboot the device. Our example app will say it is closed. How long before the service
    // catches up with that? Can the device trigger that update? It probably should... Otherwaise,
    // we have to wait for the service to request an update for the state. How often does that happen?
    mState = OPEN_CLOSE_STATE_CLOSED;
}

void SecurityOpenCloseTraitDataSource::HandleStateChange(int32_t aState)
{
    int32_t previous_state = mState;
    mState                 = aState;

    Lock();

    SetDirty(SecurityOpenCloseTrait::kPropertyHandle_OpenCloseState);
    SetDirty(SecurityOpenCloseTrait::kPropertyHandle_BypassRequested);
    SetDirty(SecurityOpenCloseTrait::kPropertyHandle_FirstObservedAtMs);

    Unlock();

    SecurityOpenCloseEvent ev;
    EventOptions options(true);
    ev.openCloseState      = mState;
    ev.priorOpenCloseState = previous_state;

    /* Bypass is not a supported feature in this example */
    ev.bypassRequested = false;
    nl::LogEvent(&ev, options);

    GetWDMFeature().ProcessTraitChanges();
}

WEAVE_ERROR SecurityOpenCloseTraitDataSource::GetLeafData(PropertyPathHandle aLeafHandle, uint64_t aTagToWrite, TLVWriter & aWriter)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    switch (aLeafHandle)
    {
    case SecurityOpenCloseTrait::kPropertyHandle_OpenCloseState: {
        err = aWriter.Put(aTagToWrite, mState);
        SuccessOrExit(err);
        break;
    }

    case SecurityOpenCloseTrait::kPropertyHandle_BypassRequested: {
        /* Bypass is not a supported feature in this example */
        bool bypass_requested = false;
        err                   = aWriter.PutBoolean(aTagToWrite, bypass_requested);
        SuccessOrExit(err);
        break;
    }

    case SecurityOpenCloseTrait::kPropertyHandle_FirstObservedAtMs: {
        uint64_t currentTime = 0;
        currentTime          = System::Platform::Layer::GetClock_MonotonicMS();
        err                  = aWriter.Put(aTagToWrite, static_cast<int64_t>(currentTime));
        SuccessOrExit(err);
        break;
    }

    default: {
        WeaveLogError(Support, "SecurityOpenCloseTrait::Unexpected Leaf");
        break;
    }
    }

exit:
    return err;
}