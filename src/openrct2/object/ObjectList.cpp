/*****************************************************************************
 * Copyright (c) 2014-2020 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ObjectList.h"

#include "../Context.h"
#include "../Game.h"
#include "../object/Object.h"
#include "../util/SawyerCoding.h"
#include "../util/Util.h"
#include "ObjectManager.h"
#include "ObjectRepository.h"

#include <algorithm>
#include <cstring>

// 98DA00
// clang-format off
int32_t object_entry_group_counts[] = {
    MAX_RIDE_OBJECTS,          // rides
    MAX_SMALL_SCENERY_OBJECTS, // small scenery
    MAX_LARGE_SCENERY_OBJECTS, // large scenery
    MAX_WALL_SCENERY_OBJECTS,  // walls
    MAX_BANNER_OBJECTS,        // banners
    MAX_PATH_OBJECTS,          // paths
    MAX_PATH_ADDITION_OBJECTS, // path bits
    MAX_SCENERY_GROUP_OBJECTS, // scenery sets
    MAX_PARK_ENTRANCE_OBJECTS, // park entrance
    MAX_WATER_OBJECTS,         // water
    MAX_SCENARIO_TEXT_OBJECTS, // scenario text
    MAX_TERRAIN_SURFACE_OBJECTS,
    MAX_TERRAIN_EDGE_OBJECTS,
    MAX_STATION_OBJECTS,
    MAX_MUSIC_OBJECTS,
};

// 98DA2C
int32_t object_entry_group_encoding[] = {
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_RLE,
    CHUNK_ENCODING_ROTATE
};
// clang-format on

bool object_entry_is_empty(const rct_object_entry* entry)
{
    uint64_t a, b;
    std::memcpy(&a, reinterpret_cast<const uint8_t*>(entry), 8);
    std::memcpy(&b, reinterpret_cast<const uint8_t*>(entry) + 8, 8);

    if (a == 0xFFFFFFFFFFFFFFFF && b == 0xFFFFFFFFFFFFFFFF)
        return true;
    if (a == 0 && b == 0)
        return true;
    return false;
}

/**
 *
 *  rct2: 0x006AB344
 */
void object_create_identifier_name(char* string_buffer, size_t size, const rct_object_entry* object)
{
    snprintf(string_buffer, size, "%.8s/%4X%4X", object->name, object->flags, object->checksum);
}

/**
 *
 *  rct2: 0x006A9DA2
 * bl = entry_index
 * ecx = entry_type
 */
bool find_object_in_entry_group(const rct_object_entry* entry, ObjectType* entry_type, ObjectEntryIndex* entryIndex)
{
    ObjectType objectType = entry->GetType();
    if (objectType >= ObjectType::Count)
    {
        return false;
    }

    auto& objectMgr = OpenRCT2::GetContext()->GetObjectManager();
    auto maxObjects = object_entry_group_counts[EnumValue(objectType)];
    for (int32_t i = 0; i < maxObjects; i++)
    {
        auto loadedObj = objectMgr.GetLoadedObject(objectType, i);
        if (loadedObj != nullptr)
        {
            auto thisEntry = object_entry_get_object(objectType, i)->GetObjectEntry();
            if (object_entry_compare(thisEntry, entry))
            {
                *entry_type = objectType;
                *entryIndex = i;
                return true;
            }
        }
    }
    return false;
}

void get_type_entry_index(size_t index, ObjectType* outObjectType, ObjectEntryIndex* outEntryIndex)
{
    uint8_t objectType = EnumValue(ObjectType::Ride);
    for (size_t groupCount : object_entry_group_counts)
    {
        if (index >= groupCount)
        {
            index -= groupCount;
            objectType++;
        }
        else
        {
            break;
        }
    }

    if (outObjectType != nullptr)
        *outObjectType = static_cast<ObjectType>(objectType);
    if (outEntryIndex != nullptr)
        *outEntryIndex = static_cast<ObjectEntryIndex>(index);
}

const rct_object_entry* get_loaded_object_entry(size_t index)
{
    ObjectType objectType;
    ObjectEntryIndex entryIndex;
    get_type_entry_index(index, &objectType, &entryIndex);

    return object_entry_get_object(objectType, entryIndex)->GetObjectEntry();
}

void* get_loaded_object_chunk(size_t index)
{
    ObjectType objectType;
    ObjectEntryIndex entryIndex;
    get_type_entry_index(index, &objectType, &entryIndex);
    return object_entry_get_chunk(objectType, entryIndex);
}

void object_entry_get_name_fixed(utf8* buffer, size_t bufferSize, const rct_object_entry* entry)
{
    bufferSize = std::min(static_cast<size_t>(DAT_NAME_LENGTH) + 1, bufferSize);
    std::memcpy(buffer, entry->name, bufferSize - 1);
    buffer[bufferSize - 1] = 0;
}

void* object_entry_get_chunk(ObjectType objectType, ObjectEntryIndex index)
{
    ObjectEntryIndex objectIndex = index;
    for (int32_t i = 0; i < EnumValue(objectType); i++)
    {
        objectIndex += object_entry_group_counts[i];
    }

    void* result = nullptr;
    auto& objectMgr = OpenRCT2::GetContext()->GetObjectManager();
    auto obj = objectMgr.GetLoadedObject(objectIndex);
    if (obj != nullptr)
    {
        result = obj->GetLegacyData();
    }
    return result;
}

const Object* object_entry_get_object(ObjectType objectType, ObjectEntryIndex index)
{
    auto& objectMgr = OpenRCT2::GetContext()->GetObjectManager();
    return objectMgr.GetLoadedObject(objectType, index);
}
