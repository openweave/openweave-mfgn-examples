
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/located/device_located_settings_trait.proto
 *
 */
#ifndef _NEST_TRAIT_LOCATED__DEVICE_LOCATED_SETTINGS_TRAIT_H_
#define _NEST_TRAIT_LOCATED__DEVICE_LOCATED_SETTINGS_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

#include <nest/trait/located/LocatedTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Located {
namespace DeviceLocatedSettingsTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum
{
    kWeaveProfileId = (0x235aU << 16) | 0x2103U
};

//
// Properties
//

enum
{
    kPropertyHandle_Root = 1,

    //---------------------------------------------------------------------------------------------------------------------------//
    //  Name                                IDL Type                            TLV Type           Optional?       Nullable?     //
    //---------------------------------------------------------------------------------------------------------------------------//

    //
    //  where_annotation_rid                weave.common.ResourceId              bytes             NO              NO
    //
    kPropertyHandle_WhereAnnotationRid = 2,

    //
    //  fixture_annotation_rid              weave.common.ResourceId              bytes             YES             NO
    //
    kPropertyHandle_FixtureAnnotationRid = 3,

    //
    //  fixture_type                        nest.trait.located.LocatedTrait.LocatedFixtureType structure         YES             NO
    //
    kPropertyHandle_FixtureType = 4,

    //
    //  major_type                          nest.trait.located.LocatedTrait.LocatedMajorFixtureType int               NO NO
    //
    kPropertyHandle_FixtureType_MajorType = 5,

    //
    //  minor_type_door                     nest.trait.located.LocatedTrait.LocatedMinorFixtureTypeDoor int               NO NO
    //
    kPropertyHandle_FixtureType_MinorTypeDoor = 6,

    //
    //  minor_type_window                   nest.trait.located.LocatedTrait.LocatedMinorFixtureTypeWindow int               NO NO
    //
    kPropertyHandle_FixtureType_MinorTypeWindow = 7,

    //
    //  minor_type_wall                     nest.trait.located.LocatedTrait.LocatedMinorFixtureTypeWall int               NO NO
    //
    kPropertyHandle_FixtureType_MinorTypeWall = 8,

    //
    //  minor_type_object                   nest.trait.located.LocatedTrait.LocatedMinorFixtureTypeObject int               NO NO
    //
    kPropertyHandle_FixtureType_MinorTypeObject = 9,

    //
    //  where_label                         weave.common.StringRef               union             NO              NO
    //
    kPropertyHandle_WhereLabel = 10,

    //
    //  where_spoken_annotation_rids        repeated weave.common.ResourceId     array             NO              NO
    //
    kPropertyHandle_WhereSpokenAnnotationRids = 11,

    //
    //  fixture_name_label                  weave.common.StringRef               union             YES             NO
    //
    kPropertyHandle_FixtureNameLabel = 12,

    //
    //  fixture_spoken_annotation_rids      repeated weave.common.ResourceId     array             YES             NO
    //
    kPropertyHandle_FixtureSpokenAnnotationRids = 13,

    //
    //  last_modified_timestamp             google.protobuf.Timestamp            uint32 seconds    NO              YES
    //
    kPropertyHandle_LastModifiedTimestamp = 14,

    //
    //  last_known_relocation_timestamp     google.protobuf.Timestamp            uint32 seconds    YES             YES
    //
    kPropertyHandle_LastKnownRelocationTimestamp = 15,

    //
    //  where_legacy_uuid                   string                               string            NO              NO
    //
    kPropertyHandle_WhereLegacyUuid = 16,

    //
    // Enum for last handle
    //
    kLastSchemaHandle = 16,
};

} // namespace DeviceLocatedSettingsTrait
} // namespace Located
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_LOCATED__DEVICE_LOCATED_SETTINGS_TRAIT_H_
