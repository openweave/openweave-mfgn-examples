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

/**
 *    @file
 *      A trait data sink implementation for the Weave Security BoltLockSettingsTrait.
 */

#include "OCSensorSecurityOpenCloseTraitDataSink.h"
#include <nest/trait/security/SecurityOpenCloseTrait.h>
#include <nest/trait/detector/OpenCloseTrait.h>

#include "DeviceController.h"

using namespace nl::Weave;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles::DataManagement;

using namespace Schema::Nest::Trait::Security;
using namespace Schema::Nest::Trait::Detector::OpenCloseTrait;
using namespace Schema::Nest::Trait::Security::SecurityOpenCloseTrait;

OCSensorSecurityOpenCloseTraitDataSink::OCSensorSecurityOpenCloseTraitDataSink() : TraitDataSink(&SecurityOpenCloseTrait::TraitSchema) {}

WEAVE_ERROR
OCSensorSecurityOpenCloseTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogError(Support, "OCSensorSecurityOpenCloseTraitDataSink::SetLeafData handle [%d]", aLeafHandle);
    switch (aLeafHandle)
    {
        case SecurityOpenCloseTrait::kPropertyHandle_OpenCloseState: {
            WeaveLogError(Support, "kPropertyHandle_OpenCloseState");
            err = aReader.Get(mState);
            SuccessOrExit(err);
            break;
        }

        case SecurityOpenCloseTrait::kPropertyHandle_BypassRequested: {
            /* Bypass is not a supported feature in this example */
            WeaveLogError(Support, "kPropertyHandle_BypassRequested");
            bool bypass_requested = false;
            err                   = aReader.Get(bypass_requested);
            SuccessOrExit(err);
            break;
        }

        case SecurityOpenCloseTrait::kPropertyHandle_FirstObservedAtMs: {
            WeaveLogError(Support, "kPropertyHandle_FirstObservedAtMs");
            uint64_t currentTime = 0;
            err                  = aReader.Get(currentTime);
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
