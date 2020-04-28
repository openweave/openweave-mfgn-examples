/*
 *
 *    Copyright (c) 2019 Google LLC.
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

#include <DeviceLocatedSettingsTraitDataSink.h>
#include <nest/trait/located/DeviceLocatedSettingsTrait.h>
#include <nest/trait/located/LocatedTrait.h>

#include <Weave/Support/CodeUtils.h>

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles::DataManagement;
using namespace ::Schema::Nest::Trait::Located;

DeviceLocatedSettingsTraitDataSink::DeviceLocatedSettingsTraitDataSink() : TraitDataSink(&DeviceLocatedSettingsTrait::TraitSchema)
{}

WEAVE_ERROR
DeviceLocatedSettingsTraitDataSink::SetLeafData(PropertyPathHandle aLeafHandle, TLVReader & aReader)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    uint32_t mFixtureMajorType = 0;
    uint32_t mFixtureMinorType = 0;

    switch (aLeafHandle)
    {
    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType_MajorType:
        err = aReader.Get(mFixtureMajorType);
        SuccessOrExit(err);

        if (mFixtureMajorType >= LocatedTrait::LOCATED_MAJOR_FIXTURE_TYPE_DOOR &&
            mFixtureMajorType <= LocatedTrait::LOCATED_MAJOR_FIXTURE_TYPE_OBJECT)
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MajorType : %d\n", mFixtureMajorType);
        }
        else
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MajorType %d is invalid\n", mFixtureMajorType);
        }
        break;

    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType_MinorTypeDoor:
        err = aReader.Get(mFixtureMinorType);
        SuccessOrExit(err);

        if (mFixtureMinorType >= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_DOOR_GENERIC &&
            mFixtureMinorType <= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_DOOR_GARAGE_SINGLE_PANEL)
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeDoor : %d\n", mFixtureMinorType);
        }
        else
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeDoor %d is invalid\n", mFixtureMinorType);
        }
        break;

    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType_MinorTypeWindow:
        err = aReader.Get(mFixtureMinorType);
        SuccessOrExit(err);

        if (mFixtureMinorType >= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_WINDOW_GENERIC &&
            mFixtureMinorType <= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_WINDOW_ROOF)
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeWindow : %d\n", mFixtureMinorType);
        }
        else
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeWindow %d is invalid\n", mFixtureMinorType);
        }
        break;

    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType_MinorTypeWall:
        err = aReader.Get(mFixtureMinorType);
        SuccessOrExit(err);

        if (mFixtureMinorType >= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_WALL_GENERIC &&
            mFixtureMinorType <= LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_WALL_FLUSH)
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeWall : %d\n", mFixtureMinorType);
        }
        else
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeWall %d is invalid\n", mFixtureMinorType);
        }

        WeaveLogDetail(Support, "<< kHandle_minor_type_wall = %d\n", mFixtureMinorType);
        break;

    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType_MinorTypeObject:
        err = aReader.Get(mFixtureMinorType);
        SuccessOrExit(err);

        if (mFixtureMinorType == LocatedTrait::LOCATED_MINOR_FIXTURE_TYPE_OBJECT_GENERIC)
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeObject : %d\n", mFixtureMinorType);
        }
        else
        {
            WeaveLogDetail(Support, "<< kPropertyHandle_FixtureType_MinorTypeObject %d is invalid\n", mFixtureMinorType);
        }
        break;

    case DeviceLocatedSettingsTrait::kPropertyHandle_FixtureType:
        break;

    default:
        WeaveLogDetail(Support, "<< UNKNOWN Device Located Settings trait leaf handle : %d\n", aLeafHandle);
    }

exit:
    return err;
}
