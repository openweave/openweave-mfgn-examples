
/**
 *    Copyright (c) 2019 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    THIS FILE IS GENERATED. DO NOT MODIFY.
 *
 *    SOURCE TEMPLATE: trait.cpp
 *    SOURCE PROTO: nest/trait/located/located_trait.proto
 *
 */

#include <nest/trait/located/LocatedTrait.h>

namespace Schema {
namespace Nest {
namespace Trait {
namespace Located {
namespace LocatedTrait {

using namespace ::nl::Weave::Profiles::DataManagement;

//
// Property Table
//

const TraitSchemaEngine::PropertyInfo PropertyMap[] = {};

//
// Schema
//

const TraitSchemaEngine TraitSchema = { {
    kWeaveProfileId,
    PropertyMap,
    sizeof(PropertyMap) / sizeof(PropertyMap[0]),
    1,
#if (TDM_EXTENSION_SUPPORT) || (TDM_VERSIONING_SUPPORT)
    2,
#endif
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#if (TDM_EXTENSION_SUPPORT)
    NULL,
#endif
#if (TDM_VERSIONING_SUPPORT)
    NULL,
#endif
} };

//
// Event Structs
//

const nl::FieldDescriptor LocatedFixtureTypeFieldDescriptors[] = {
    { NULL, offsetof(LocatedFixtureType, majorType), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 1 },

    { NULL, offsetof(LocatedFixtureType, minorTypeDoor), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 2 },

    { NULL, offsetof(LocatedFixtureType, minorTypeWindow), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 3 },

    { NULL, offsetof(LocatedFixtureType, minorTypeWall), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 4 },

    { NULL, offsetof(LocatedFixtureType, minorTypeObject), SET_TYPE_AND_FLAGS(nl::SerializedFieldTypeInt32, 0), 5 },

};

const nl::SchemaFieldDescriptor LocatedFixtureType::FieldSchema = {
    .mNumFieldDescriptorElements = sizeof(LocatedFixtureTypeFieldDescriptors) / sizeof(LocatedFixtureTypeFieldDescriptors[0]),
    .mFields                     = LocatedFixtureTypeFieldDescriptors,
    .mSize                       = sizeof(LocatedFixtureType)
};

} // namespace LocatedTrait
} // namespace Located
} // namespace Trait
} // namespace Nest
} // namespace Schema
