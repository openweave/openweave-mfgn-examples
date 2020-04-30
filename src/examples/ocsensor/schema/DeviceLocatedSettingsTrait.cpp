
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/located/device_located_settings_trait.proto
 *
 */

#include <nest/trait/located/DeviceLocatedSettingsTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Located {
namespace DeviceLocatedSettingsTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {
    { kPropertyHandle_Root, 2 },        // where_annotation_rid
    { kPropertyHandle_Root, 3 },        // fixture_annotation_rid
    { kPropertyHandle_Root, 4 },        // fixture_type
    { kPropertyHandle_FixtureType, 1 }, // major_type
    { kPropertyHandle_FixtureType, 2 }, // minor_type_door
    { kPropertyHandle_FixtureType, 3 }, // minor_type_window
    { kPropertyHandle_FixtureType, 4 }, // minor_type_wall
    { kPropertyHandle_FixtureType, 5 }, // minor_type_object
    { kPropertyHandle_Root, 5 },        // where_label
    { kPropertyHandle_Root, 6 },        // where_spoken_annotation_rids
    { kPropertyHandle_Root, 7 },        // fixture_name_label
    { kPropertyHandle_Root, 8 },        // fixture_spoken_annotation_rids
    { kPropertyHandle_Root, 9 },        // last_modified_timestamp
    { kPropertyHandle_Root, 10 },       // last_known_relocation_timestamp
    { kPropertyHandle_Root, 11 },       // where_legacy_uuid
};

//
// IsOptional Table
//

uint8_t IsOptionalHandleBitfield[] = { 0x6, 0x2c };

//
// IsNullable Table
//

uint8_t IsNullableHandleBitfield[] = { 0x0, 0x30 };

//
// Supported version
//
const ConstSchemaVersionRange traitVersion = { .mMinVersion = 1, .mMaxVersion = 2 };

//
// Schema
//

const TraitSchemaEngine TraitSchema = { {
    kWeaveProfileId,
    PropertyMap,
    sizeof(PropertyMap) / sizeof(PropertyMap[0]),
    2,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
    2,
#endif
    NULL,
    &IsOptionalHandleBitfield[0],
    NULL,
    &IsNullableHandleBitfield[0],
    NULL,
#if (TDM_EXTENSION_SUPPORT)
    NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
    &traitVersion,
#endif
} };

} // namespace DeviceLocatedSettingsTrait
} // namespace Located
} // namespace Trait
} // namespace Nest
} // namespace Schema
