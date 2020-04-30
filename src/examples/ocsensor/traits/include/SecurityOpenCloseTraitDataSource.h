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

#ifndef SECURITY_OPEN_CLOSE_TRAIT_DATA_SOURCE_H
#define SECURITY_OPEN_CLOSE_TRAIT_DATA_SOURCE_H

#include <stdint.h>
#include <stdbool.h>

#include <Weave/Profiles/data-management/TraitData.h>

class SecurityOpenCloseTraitDataSource : public ::nl::Weave::Profiles::DataManagement_Current::TraitDataSource
{
public:
    SecurityOpenCloseTraitDataSource(void);

    void HandleStateChange(int32_t aState);

private:
    WEAVE_ERROR GetLeafData(::nl::Weave::Profiles::DataManagement_Current::PropertyPathHandle aLeafHandle, uint64_t aTagToWrite,
                            ::nl::Weave::TLV::TLVWriter & aWriter) override;

    int32_t mState;
};

#endif // SECURITY_OPEN_CLOSE_TRAIT_DATA_SOURCE_H