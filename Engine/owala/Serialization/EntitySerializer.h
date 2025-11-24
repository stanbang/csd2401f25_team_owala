/*****************************************************************//**
* \file   EntitySerializer.h
*
* \primary_author
*       Jason Lou (70%)
*
* \secondary_author
*       Yar Xuan (30%)
*
* \brief    This file defines the EntitySerializer class, which provides a static interface for 
            serializing (saving) and deserializing (loading) game entities, their components, 
            and entire scenes using the RapidJSON library
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
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
    public:

        /* Serialization APIs */
        // Serialize an entity and all its components into a JSON object.
        static void SerializeEntity(Entity entity, EntityStorage& ecs, rapidjson::Value& outData, rapidjson::Document::AllocatorType& allocator);
        // Serializes an entity to a JSON file.
        static void SerializeEntityToFile(Entity entity, EntityStorage& ecs, const std::string& filepath);
        // Serializes multiple entities to outArray.
        static void SerializeEntities(const std::vector<Entity>& entities, EntityStorage& ecs, rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc);
        // Serialize properties of a given entity into a JSON object.
        static void SerializeEntityProperties(EntityManager& entityMgr, Entity entity, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        // Serialize tags for a given entity into a JSON object.
        static void SerializeTags(EntityManager& entityMgr, Entity entity, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        // Serializes scene data to a JSON file.
        static void SerializeSceneData(const std::vector<Entity>& entities, EntityStorage& ecs, const std::string& filePath, const std::string& sceneName);
        // Serializes the current state of an entity to a JSON Object and return as a string.
        static std::string SerializeEntityToString(Entity entity, EntityStorage& ecs);
        // Serializes the current state of the entity properties into a JSON Object and return as a string. 
        static std::string SerializeEntityPropertiesToString(Entity entity, EntityStorage& ecs);
        // Serializes the current state of a component to a JSON Object and return as a string. Returns an empty string if component does not exist.
        static std::string SerializeComponentToString(Entity entity, ComponentType compType, EntityStorage& ecs);
        // Serializes the current tags of an entity to a JSON Object and return as a string. Returns an empty string if theres no tags on the entity.
        static std::string SerializeTagsToString(Entity entity, EntityStorage& ecs);

		/* Deserialization APIs */
        // Deserialize a new entity from JSON (creates a new entity and adds components).
        static Entity DeserializeEntity(EntityStorage& ecs, const rapidjson::Value& entityData);
		// Deserialize an entity to an existing entity (modifies the existing entity).
		static void DeserializeEntityToExisting(EntityStorage& ecs, Entity entityToOverwrite, const rapidjson::Value& entityData);
        // Deserializes an entity's properties from JSON data.
        static void DeserializeEntityProperties(const rapidjson::Value& entityData, EntityProperties& out);
		// Deserialize components for a given entity from JSON data.
        static void DeserializeComponents(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData);
		// Deserialize tags for a given entity from JSON data.
        static void DeserializeTags(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData);
        // Deserialize scene data from JSON file (modifies sceneName and entitiesList)
        static bool DeserializeSceneData(EntityStorage& ecs, const std::string& filePath, std::string& sceneName, std::vector<Entity>& entitiesList);

        /* Specialized Serialization APIs */
        /* Components */

        static void SerializeComponent(const Position& pos, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const SpawnPoint& spawnPt, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Velocity& vel, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Acceleration& acc, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Triangle& triangle, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const RigidBody& rb, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const AABBCollider& col, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const PlayerState& state, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const RotationFlag& rFlag, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const TrapPlatformComponent& trap, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const ScalingPlatformComponent& scalingPlatform, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const TextureComponent& texComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const AudioComponent& audioComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const AnimationComponent& animComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const TextComponent& textComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Camera& cameraComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const NativeScriptsComponent& nativeScripts, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const BirdAIComponent& birdAI, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const SkeletonAIComponent& skelAI, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Button& button, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const SceneOwned& sceneOwnedComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const ParticleEmitterComponent& particleEmitter, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Rotation& rotationComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);
        static void SerializeComponent(const Scale& scaleComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc);

        /* Specialized Deserialization APIs */
        /* Components */

        static bool DeserializeComponent(const rapidjson::Value& componentData, Position& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, SpawnPoint& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, Velocity& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, Acceleration& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, Triangle& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, RigidBody& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, AABBCollider& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, PlayerState& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, RotationFlag& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, TrapPlatformComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, ScalingPlatformComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, TextureComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, AudioComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, AnimationComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, TextComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, Camera& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, NativeScriptsComponent& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, BirdAIComponent& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, SkeletonAIComponent& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, Button& out);
		static bool DeserializeComponent(const rapidjson::Value& componentData, SceneOwned& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, ParticleEmitterComponent& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, Rotation& out);
        static bool DeserializeComponent(const rapidjson::Value& componentData, Scale& out);

    };
}