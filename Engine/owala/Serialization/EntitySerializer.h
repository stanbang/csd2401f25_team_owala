#pragma once
#include "Entity.h"
#include "EntityStorage.h"

// JSON Serialization
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace Framework {
    class EntitySerializer {
    private:
        // Main Serialization Logic
        static void SerializeEntityInternal(Entity entity, EntityStorage& ecs, rapidjson::Value& value, rapidjson::Document::AllocatorType& alloc);
        static void DeserializeComponents(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData);
        static void DeserializeTags(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData);
    public:

        /* Serialization APIs */
        // Serialize an entity and all its components into a JSON object.
        static void SerializeEntity(Entity entity, EntityStorage& ecs, rapidjson::Value& outData, rapidjson::Document::AllocatorType& allocator);
        // Serializes an entity to a JSON file.
        static void SerializeEntityToFile(Entity entity, EntityStorage& ecs, const std::string& filepath);
        // Serializes multiple entities to outArray.
        static void SerializeEntities(const std::vector<Entity>& entities, EntityStorage& ecs, rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc);
        // Serializes scene data to a JSON file.
        static void SerializeSceneData(const std::vector<Entity>& entities, EntityStorage& ecs, const std::string& filePath, const std::string& sceneName);

        // Deserialize a new entity from JSON (creates a new entity and adds components).
        static Entity DeserializeEntity(EntityStorage& ecs, const rapidjson::Value& entityData);
        // Deserialize scene data from JSON file (modifies sceneName and entitiesList)
        static bool DeserializeSceneData(EntityStorage& ecs, const std::string& filePath, std::string& sceneName, std::vector<Entity>& entitiesList);

    };
}