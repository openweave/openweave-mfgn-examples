
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp.h
 *    SOURCE PROTO: nest/trait/located/located_trait.proto
 *
 */
#ifndef _NEST_TRAIT_LOCATED__LOCATED_TRAIT_H_
#define _NEST_TRAIT_LOCATED__LOCATED_TRAIT_H_

#include <Weave/Profiles/data-management/DataManagement.h>
#include <Weave/Support/SerializationUtils.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Located {
namespace LocatedTrait {

extern const nl::Weave::Profiles::DataManagement::TraitSchemaEngine TraitSchema;

enum
{
    kWeaveProfileId = (0x235aU << 16) | 0x2102U
};

//
// Event Structs
//

struct LocatedFixtureType
{
    int32_t majorType;
    int32_t minorTypeDoor;
    int32_t minorTypeWindow;
    int32_t minorTypeWall;
    int32_t minorTypeObject;

    static const nl::SchemaFieldDescriptor FieldSchema;
};

struct LocatedFixtureType_array
{
    uint32_t num;
    LocatedFixtureType * buf;
};

//
// Enums
//

enum LocatedMajorFixtureType
{
    LOCATED_MAJOR_FIXTURE_TYPE_DOOR   = 1,
    LOCATED_MAJOR_FIXTURE_TYPE_WINDOW = 2,
    LOCATED_MAJOR_FIXTURE_TYPE_WALL   = 3,
    LOCATED_MAJOR_FIXTURE_TYPE_OBJECT = 4,
};

enum LocatedMinorFixtureTypeDoor
{
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_GENERIC             = 1,
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_HINGED              = 2,
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_FRENCH              = 3,
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_SLIDING             = 4,
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_GARAGE_SEGMENTED    = 5,
    LOCATED_MINOR_FIXTURE_TYPE_DOOR_GARAGE_SINGLE_PANEL = 6,
};

enum LocatedMinorFixtureTypeWindow
{
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_GENERIC                = 1,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_VERTICAL_SINGLE_HUNG   = 2,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_HORIZONTAL_SINGLE_HUNG = 3,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_VERTICAL_DOUBLE_HUNG   = 4,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_HORIZONTAL_DOUBLE_HUNG = 5,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_VERTICAL_CASEMENT      = 6,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_HORIZONTAL_CASEMENT    = 7,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_TILTTURN               = 8,
    LOCATED_MINOR_FIXTURE_TYPE_WINDOW_ROOF                   = 9,
};

enum LocatedMinorFixtureTypeWall
{
    LOCATED_MINOR_FIXTURE_TYPE_WALL_GENERIC = 1,
    LOCATED_MINOR_FIXTURE_TYPE_WALL_CORNER  = 2,
    LOCATED_MINOR_FIXTURE_TYPE_WALL_FLUSH   = 3,
};

enum LocatedMinorFixtureTypeObject
{
    LOCATED_MINOR_FIXTURE_TYPE_OBJECT_GENERIC = 1,
};

} // namespace LocatedTrait
} // namespace Located
} // namespace Trait
} // namespace Nest
} // namespace Schema
#endif // _NEST_TRAIT_LOCATED__LOCATED_TRAIT_H_
