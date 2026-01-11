/*****************************************************************//**
* \file   EntitySerializer.cpp
*
 * \primary_author
 *		Lou Jason (50%)
 * \secondary_author
 *      Lur Pei Hao Stanley (10%)
 *      Sitara Anil (10%)
 *      Ng Jordain Ying Zhe (10%)
 *      Ng Jing Wen (10%)
 *      Yar Xuan (10%)
*		
*
* \brief    This file defines the EntitySerializer class, which provides a static interface for
			serializing (saving) and deserializing (loading) game entities, their components,
			and entire scenes using the RapidJSON library
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#include "EntitySerializer.h"
#include "SerializationKeys.h"
#include "EntityManager.h"
#include "ScriptRegistry.h"
#include "FileUtility.h"
#include "Logger.h"

// Internal Helpers - Will move this and the Editor's version to a global utils file later.
namespace {
	using namespace Framework;
}

namespace Framework {
	void EntitySerializer::SerializeEntityInternal(Entity entity, EntityStorage& ecs, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		EntityManager entityMgr(ecs);

		/* Entity Properties Serialization */
		SerializeEntityProperties(entityMgr, entity, outData, alloc);

		/* Components Serialization */
		SerializeComponents(entityMgr, entity, outData, alloc);

		/* Tags Serialization */
		SerializeTags(entityMgr, entity, outData, alloc);
	}

	void EntitySerializer::SerializeEntity(Entity entity, EntityStorage& ecs, rapidjson::Value& outData, rapidjson::Document::AllocatorType& allocator) {
		SerializeEntityInternal(entity, ecs, outData, allocator);
	}

	void EntitySerializer::SerializeEntityToFile(Entity entity, EntityStorage& ecs, const std::string& filepath) {
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Value outData;

		Logger::Get().Info(DebugType::EntitySeralizer) << "Serializing entity to " << filepath << std::endl;
		SerializeEntityInternal(entity, ecs, doc, doc.GetAllocator());
		FileIOSystem::WriteFileJson(filepath, doc);
		Logger::Get().Info(DebugType::EntitySeralizer) << "Successfully saved entity to " << filepath << std::endl;
	}

	void EntitySerializer::SerializeEntities(const std::vector<Entity>& entities, EntityStorage& ecs, rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc) {
		for (Entity entity : entities)
		{
			rapidjson::Value entityObject(rapidjson::kObjectType);
			SerializeEntityInternal(entity, ecs, entityObject, alloc);
			outArray.PushBack(entityObject, alloc);
		}
	}

	void EntitySerializer::SerializeEntityProperties(EntityManager& entityMgr, Entity entity, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		EntityProperties& properties = entityMgr.GetProperties(entity);

		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_NAME), rapidjson::StringRef(properties.name.c_str()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_LAYER), static_cast<int>(properties.layer), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_RENDER_LAYER), static_cast<int>(properties.renderLayer), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_CUSTOM_RENDER_ORDER), properties.customRenderOrder, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_IS_ACTIVE), properties.isActive, alloc);
	}

	void EntitySerializer::SerializeComponents(EntityManager& entityMgr, Entity entity, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value componentList(rapidjson::kObjectType);

		// Serialize Transform Component
		if (entityMgr.HasComponent(entity, ComponentType::TransformType)) {
			Transform& transform = entityMgr.GetComponent<Transform>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(transform, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TRANSFORM_COMPONENT), value, alloc);
		}

		// Serialize SpawnPoint Component
		if (entityMgr.HasComponent(entity, ComponentType::SpawnPointType)) {
			SpawnPoint& spawnPt = entityMgr.GetComponent<SpawnPoint>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(spawnPt, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::SPAWN_PT_COMPONENT), value, alloc);
		}

		// Serialize Velocity Component
		if (entityMgr.HasComponent(entity, ComponentType::VelocityType)) {
			Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(vel, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::VELOCITY_COMPONENT), value, alloc);
		}

		// Serialize Acceleration Component
		if (entityMgr.HasComponent(entity, ComponentType::AccelerationType)) {
			Acceleration& acc = entityMgr.GetComponent<Acceleration>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(acc, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ACCELERATION_COMPONENT), value, alloc);
		}

		// Serialize Triangle Component
		if (entityMgr.HasComponent(entity, ComponentType::TriangleType)) {
			Triangle& triangle = entityMgr.GetComponent<Triangle>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(triangle, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TRIANGLE_COMPONENT), value, alloc);
		}

		// Serialize RigidBody Component
		if (entityMgr.HasComponent(entity, ComponentType::RigidBodyType)) {
			RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(rb, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::RIGIDBODY_COMPONENT), value, alloc);
		}

		// Serialize AABBCollider Component
		if (entityMgr.HasComponent(entity, ComponentType::AABBColliderType)) {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(col, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::AABB_COLLIDER_COMPONENT), value, alloc);
		}

		// Serialize PlayerState Component
		if (entityMgr.HasComponent(entity, ComponentType::PlayerStateType)) {
			PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(state, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::PLAYER_STATE_COMPONENT), value, alloc);
		}

		// Serialize RotationFlag Component
		if (entityMgr.HasComponent(entity, ComponentType::RotationFlagType)) {
			RotationFlag& rFlag = entityMgr.GetComponent<RotationFlag>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(rFlag, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ROTATIONFLAG_COMPONENT), value, alloc);
		}

		// Serialize TrapPlatform Component
		if (entityMgr.HasComponent(entity, ComponentType::TrapPlatformComponentType)) {
			TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(trap, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TRAP_PLATFORM_COMPONENT), value, alloc);
		}

		// Serialize TextureComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextureComponentType)) {
			TextureComponent& texComp = entityMgr.GetComponent<TextureComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(texComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TEXTURE_COMPONENT), value, alloc);
		}

		// Serialize AudioComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AudioComponentType)) {
			AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(audioComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::AUDIO_COMPONENT), value, alloc);
		}

		// Serialize AnimationComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AnimationComponentType)) {
			AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(animComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ANIMATION_COMPONENT), value, alloc);
		}

		// Serialize TextComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextComponentType)) {
			TextComponent& textComp = entityMgr.GetComponent<TextComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(textComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TEXT_COMPONENT), value, alloc);
		}

		// Serialize Camera Component
		if (entityMgr.HasComponent(entity, ComponentType::CameraType)) {
			Camera& cameraComp = entityMgr.GetComponent<Camera>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(cameraComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::CAMERA_COMPONENT), value, alloc);
		}

		// Serialize Native Scripts that is attached to this entity.
		if (entityMgr.HasComponent(entity, ComponentType::NativeScriptsComponentType)) {
			NativeScriptsComponent& nativeScripts = entityMgr.GetComponent<NativeScriptsComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(nativeScripts, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::NATIVE_SCRIPTS_COMPONENT), value, alloc);
		}

		if (entityMgr.HasComponent(entity, ComponentType::BirdAIComponentType)) {
			BirdAIComponent& birdAI = entityMgr.GetComponent<BirdAIComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);

			SerializeComponent(birdAI, value, alloc);

			// currentPath is runtime state, don't serialize it

			componentList.AddMember(rapidjson::StringRef(Serialization::BIRD_AI_COMPONENT), value, alloc);
		}

		if (entityMgr.HasComponent(entity, ComponentType::SkeletonAIComponentType)) {
			SkeletonAIComponent& skelAI = entityMgr.GetComponent<SkeletonAIComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);

			SerializeComponent(skelAI, value, alloc);

			componentList.AddMember(rapidjson::StringRef(Serialization::SKELETON_AI_COMPONENT), value, alloc);
		}

		// Serialize Button Component
		if (entityMgr.HasComponent(entity, ComponentType::ButtonType)) {
			Button& button = entityMgr.GetComponent<Button>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(button, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::BUTTON_COMPONENT), value, alloc);
		}

		// Serialize SceneOwned Component
		if (entityMgr.HasComponent(entity, ComponentType::SceneOwnedType)) {
			SceneOwned& sceneOwnedComp = entityMgr.GetComponent<SceneOwned>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(sceneOwnedComp, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::SCENE_OWNED_COMPONENT), value, alloc);
		}

		// Serialize ParticleEmitter Component
		if (entityMgr.HasComponent(entity, ComponentType::ParticleEmitterComponentType)) {
			ParticleEmitterComponent& particleEmitter = entityMgr.GetComponent<ParticleEmitterComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			SerializeComponent(particleEmitter, value, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::PARTICLE_EMITTER_COMPONENT), value, alloc);
		}

		outData.AddMember(rapidjson::StringRef(Serialization::COMPONENTS), componentList, alloc);
	}

	void EntitySerializer::SerializeTags(EntityManager& entityMgr, Entity entity, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		// Serialize Tags into an array
		rapidjson::Value tagsList(rapidjson::kArrayType);

		if (entityMgr.HasComponent(entity, ComponentType::PlayerTagType)) {
			rapidjson::Value tagVal(Serialization::PLAYER_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::PlatformTagType)) {
			rapidjson::Value tagVal(Serialization::PLATFORM_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::BackgroundTagType)) {
			rapidjson::Value tagVal(Serialization::BACKGROUND_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::EnemyTagType)) {
			rapidjson::Value tagVal(Serialization::ENEMY_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::PersistentTagType)) {
			rapidjson::Value tagVal(Serialization::PERSISTENT_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::ActiveTagType)) {
			rapidjson::Value tagVal(Serialization::ACTIVE_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::GUITagType)) {
			rapidjson::Value tagVal(Serialization::GUI_TAG, alloc);
			tagsList.PushBack(tagVal, alloc);
		}

		outData.AddMember(rapidjson::StringRef(Serialization::TAGS), tagsList, alloc);
	}

	void EntitySerializer::SerializeSceneData(const std::vector<Entity>& entities, EntityStorage& ecs, const std::string& filePath, const std::string& sceneName) {
		try {
			Logger::Get().Info(DebugType::EntitySeralizer) << "Serializing scene <" << sceneName << "> to: " << filePath << std::endl;
			rapidjson::Document doc;
			doc.SetObject();
			rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
			rapidjson::Value sceneNameValue(sceneName.c_str(), alloc);
			doc.AddMember(rapidjson::StringRef(Serialization::SCENE_NAME), sceneNameValue, alloc);

			rapidjson::Value entitiesData(rapidjson::kArrayType);
			SerializeEntities(entities, ecs, entitiesData, alloc);
			doc.AddMember(rapidjson::StringRef(Serialization::ENTITIES), entitiesData, alloc);

			FileIOSystem::WriteFileJson(filePath, doc);
			Logger::Get().Info(DebugType::EntitySeralizer) << "Successfully saved scene <" << sceneName << "> to: " << filePath << std::endl;
		}
		catch (...) {
			Logger::Get().Error(DebugType::EntitySeralizer) << "Failed to save scene <" << sceneName << "> to: " << filePath << std::endl;
		}
	}

	std::string EntitySerializer::SerializeEntityToString(Entity entity, EntityStorage& ecs) {
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		rapidjson::Value entityData(rapidjson::kObjectType);
		SerializeEntity(entity, ecs, entityData, alloc);
		doc.CopyFrom(entityData, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return std::string(buffer.GetString());
	}

	std::string EntitySerializer::SerializeEntityPropertiesToString(Entity entity, EntityStorage& ecs) {
		EntityManager entityMgr(ecs);
		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		rapidjson::Value propertiesData(rapidjson::kObjectType);
		SerializeEntityProperties(entityMgr, entity, propertiesData, alloc);
		doc.CopyFrom(propertiesData, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return std::string(buffer.GetString());
	}

	std::string EntitySerializer::SerializeComponentToString(Entity entity, ComponentType compType, EntityStorage& ecs) {
		EntityManager entityMgr(ecs);
		if (!entityMgr.HasComponent(entity, compType)) return {}; // Entity does not have the component!

		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		rapidjson::Value componentData(rapidjson::kObjectType);
		switch (compType) {
		case TransformType:
			SerializeComponent(entityMgr.GetComponent<Transform>(entity), componentData, alloc);
			break;
		case SpawnPointType:
			SerializeComponent(entityMgr.GetComponent<SpawnPoint>(entity), componentData, alloc);
			break;
		case VelocityType:
			SerializeComponent(entityMgr.GetComponent<Velocity>(entity), componentData, alloc);
			break;
		case AccelerationType:
			SerializeComponent(entityMgr.GetComponent<Acceleration>(entity), componentData, alloc);
			break;
		case TriangleType:
			SerializeComponent(entityMgr.GetComponent<Triangle>(entity), componentData, alloc);
			break;
		case RigidBodyType:
			SerializeComponent(entityMgr.GetComponent<RigidBody>(entity), componentData, alloc);
			break;
		case AABBColliderType:
			SerializeComponent(entityMgr.GetComponent<AABBCollider>(entity), componentData, alloc);
			break;
		case PlayerStateType:
			SerializeComponent(entityMgr.GetComponent<PlayerState>(entity), componentData, alloc);
			break;
		case RotationFlagType:
			SerializeComponent(entityMgr.GetComponent<RotationFlag>(entity), componentData, alloc);
			break;
		case TrapPlatformComponentType:
			SerializeComponent(entityMgr.GetComponent<TrapPlatformComponent>(entity), componentData, alloc);
			break;
		case TextureComponentType:
			SerializeComponent(entityMgr.GetComponent<TextureComponent>(entity), componentData, alloc);
			break;
		case AudioComponentType:
			SerializeComponent(entityMgr.GetComponent<AudioComponent>(entity), componentData, alloc);
			break;
		case AnimationComponentType:
			SerializeComponent(entityMgr.GetComponent<AnimationComponent>(entity), componentData, alloc);
			break;
		case TextComponentType:
			SerializeComponent(entityMgr.GetComponent<TextComponent>(entity), componentData, alloc);
			break;
		case CameraType:
			SerializeComponent(entityMgr.GetComponent<Camera>(entity), componentData, alloc);
			break;
		case NativeScriptsComponentType:
			SerializeComponent(entityMgr.GetComponent<NativeScriptsComponent>(entity), componentData, alloc);
			break;
		case ButtonType:
			SerializeComponent(entityMgr.GetComponent<Button>(entity), componentData, alloc);
			break;
		case BirdAIComponentType:
			SerializeComponent(entityMgr.GetComponent<BirdAIComponent>(entity), componentData, alloc);
			break;
		case SkeletonAIComponentType:
			SerializeComponent(entityMgr.GetComponent<SkeletonAIComponent>(entity), componentData, alloc);
			break;
		case SceneOwnedType:
			SerializeComponent(entityMgr.GetComponent<SceneOwned>(entity), componentData, alloc);
			break;
		case ParticleEmitterComponentType:
			SerializeComponent(entityMgr.GetComponent<ParticleEmitterComponent>(entity), componentData, alloc);
			break;
		}
		doc.CopyFrom(componentData, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return std::string(buffer.GetString());
	}

	std::string EntitySerializer::SerializeTagsToString(Entity entity, EntityStorage& ecs) {
		EntityManager entityMgr(ecs);

		bool hasAnyTags = false;
		for (int i = 0; i < static_cast<int>(ComponentType::MAX_COMPONENTS); ++i) {
			ComponentType type = static_cast<ComponentType>(i);
			if (!entityMgr.IsTagComponent(type)) {
				continue;
			}

			if (entityMgr.HasComponent(entity, type)) {
				hasAnyTags = true;
				break;
			}
		}

		if (!hasAnyTags) return {};

		rapidjson::Document doc;
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

		SerializeTags(entityMgr, entity, doc, alloc);

		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);

		return std::string(buffer.GetString());
	}

	void EntitySerializer::DeserializeComponents(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData) {
		const rapidjson::Value& componentLists = entityData[Serialization::COMPONENTS];

		// Deserialize Transform Component
		if (componentLists.HasMember(Serialization::TRANSFORM_COMPONENT)) {
			const rapidjson::Value& transformData = componentLists[Serialization::TRANSFORM_COMPONENT];

			Transform transform;
			if (DeserializeComponent(transformData, transform)) {
				entityMgr.AddComponent<Transform>(loadedEntity, transform);
			}
		}

		// Deserialize SpawnPoint Component
		if (componentLists.HasMember(Serialization::SPAWN_PT_COMPONENT)) {
			const rapidjson::Value& pos = componentLists[Serialization::SPAWN_PT_COMPONENT];

			SpawnPoint spawnPt;
			if (DeserializeComponent(pos, spawnPt)) {
				entityMgr.AddComponent<SpawnPoint>(loadedEntity, spawnPt);
			}
		}

		// Deserialize Velocity Component
		if (componentLists.HasMember(Serialization::VELOCITY_COMPONENT)) {
			const rapidjson::Value& vel = componentLists[Serialization::VELOCITY_COMPONENT];

			Velocity velocity;
			if (DeserializeComponent(vel, velocity)) {
				entityMgr.AddComponent<Velocity>(loadedEntity, velocity);
			}
		}

		// Deserialize Acceleration Component
		if (componentLists.HasMember(Serialization::ACCELERATION_COMPONENT)) {
			const rapidjson::Value& acc = componentLists[Serialization::ACCELERATION_COMPONENT];

			Acceleration acceleration;
			if (DeserializeComponent(acc, acceleration)) {
				entityMgr.AddComponent<Acceleration>(loadedEntity, acceleration);
			}
		}

		// Deserialize Triangle Component
		if (componentLists.HasMember(Serialization::TRIANGLE_COMPONENT)) {
			const rapidjson::Value& triangleData = componentLists[Serialization::TRIANGLE_COMPONENT];

			Triangle triangle;
			if (DeserializeComponent(triangleData, triangle)) {
				entityMgr.AddComponent<Triangle>(loadedEntity, triangle);
			}
		}

		// Deserialize RigidBody Component
		if (componentLists.HasMember(Serialization::RIGIDBODY_COMPONENT)) {
			const rapidjson::Value& rbData = componentLists[Serialization::RIGIDBODY_COMPONENT];

			RigidBody rigidBody;
			if (DeserializeComponent(rbData, rigidBody)) {
				entityMgr.AddComponent<RigidBody>(loadedEntity, rigidBody);
			}
		}

		// Deserialize AABBCollider Component
		if (componentLists.HasMember(Serialization::AABB_COLLIDER_COMPONENT)) {
			const rapidjson::Value& colData = componentLists[Serialization::AABB_COLLIDER_COMPONENT];

			AABBCollider aabbCollider;
			if (DeserializeComponent(colData, aabbCollider)) {
				entityMgr.AddComponent<AABBCollider>(loadedEntity, aabbCollider);
			}
		}

		// Deserialize PlayerState Component
		if (componentLists.HasMember(Serialization::PLAYER_STATE_COMPONENT)) {
			const rapidjson::Value& playerStateData = componentLists[Serialization::PLAYER_STATE_COMPONENT];

			PlayerState playerState;
			if (DeserializeComponent(playerStateData, playerState)) {
				entityMgr.AddComponent<PlayerState>(loadedEntity, playerState);
			}
		}

		// Deserialize RotationFlag Component
		if (componentLists.HasMember(Serialization::ROTATIONFLAG_COMPONENT)) {
			const rapidjson::Value& rFlagData = componentLists[Serialization::ROTATIONFLAG_COMPONENT];

			RotationFlag rotationFlag;
			if (DeserializeComponent(rFlagData, rotationFlag)) {
				entityMgr.AddComponent<RotationFlag>(loadedEntity, rotationFlag);
			}
		}

		// Deserialize TrapPlatform Component
		if (componentLists.HasMember(Serialization::TRAP_PLATFORM_COMPONENT)) {
			const rapidjson::Value& trapData = componentLists[Serialization::TRAP_PLATFORM_COMPONENT];

			TrapPlatformComponent trapPlatform;
			if (DeserializeComponent(trapData, trapPlatform)) {
				entityMgr.AddComponent<TrapPlatformComponent>(loadedEntity, trapPlatform);
			}
		}

		// Deserialize TextureComponent Component
		if (componentLists.HasMember(Serialization::TEXTURE_COMPONENT)) {
			const rapidjson::Value& texData = componentLists[Serialization::TEXTURE_COMPONENT];

			TextureComponent textureComp;
			if (DeserializeComponent(texData, textureComp)) {
				entityMgr.AddComponent<TextureComponent>(loadedEntity, textureComp);
			}
		}

		// Deserialize AudioComponent Component
		if (componentLists.HasMember(Serialization::AUDIO_COMPONENT)) {
			const rapidjson::Value& audioData = componentLists[Serialization::AUDIO_COMPONENT];

			AudioComponent audioComp;
			if (DeserializeComponent(audioData, audioComp)) {
				entityMgr.AddComponent<AudioComponent>(loadedEntity, audioComp);
			}
		}

		// Deserialize AnimationComponent Component
		if (componentLists.HasMember(Serialization::ANIMATION_COMPONENT)) {
			const rapidjson::Value& animData = componentLists[Serialization::ANIMATION_COMPONENT];

			AnimationComponent animComp;
			if (DeserializeComponent(animData, animComp)) {
				entityMgr.AddComponent<AnimationComponent>(loadedEntity, animComp);
			}
		}

		// Deserialize TextComponent Component
		if (componentLists.HasMember(Serialization::TEXT_COMPONENT)) {
			const rapidjson::Value& textData = componentLists[Serialization::TEXT_COMPONENT];

			TextComponent textComp;
			if (DeserializeComponent(textData, textComp)) {
				entityMgr.AddComponent<TextComponent>(loadedEntity, textComp);
			}
		}

		// Deserialize Camera Component
		if (componentLists.HasMember(Serialization::CAMERA_COMPONENT)) {
			const rapidjson::Value& cameraData = componentLists[Serialization::CAMERA_COMPONENT];

			Camera cameraComp;
			if (DeserializeComponent(cameraData, cameraComp)) {
				entityMgr.AddComponent<Camera>(loadedEntity, cameraComp);
			}
		}

		// Deserialize NativeScripts
		if (componentLists.HasMember(Serialization::NATIVE_SCRIPTS_COMPONENT)) {
			const rapidjson::Value& savedNativeScripts = componentLists[Serialization::NATIVE_SCRIPTS_COMPONENT];

			NativeScriptsComponent nativeScripts;
			if (DeserializeComponent(savedNativeScripts, nativeScripts)) {
				entityMgr.AddComponent<NativeScriptsComponent>(loadedEntity, std::move(nativeScripts));
			}
		}

		// Deserialize BirdAI Component
		if (componentLists.HasMember(Serialization::BIRD_AI_COMPONENT)) {
			const rapidjson::Value& birdAIData = componentLists[Serialization::BIRD_AI_COMPONENT];

			BirdAIComponent birdAI;
			if (DeserializeComponent(birdAIData, birdAI)) {
				entityMgr.AddComponent<BirdAIComponent>(loadedEntity, birdAI);
			}
		}

		// Deserialize SkeletonAI Component
		if (componentLists.HasMember(Serialization::SKELETON_AI_COMPONENT)) {
			const rapidjson::Value& skelAIData = componentLists[Serialization::SKELETON_AI_COMPONENT];

			SkeletonAIComponent skelAI;
			if (DeserializeComponent(skelAIData, skelAI)) {
				entityMgr.AddComponent<SkeletonAIComponent>(loadedEntity, skelAI);
			}
		}

		// Deserialize Button Component
		if (componentLists.HasMember(Serialization::BUTTON_COMPONENT)) {
			const rapidjson::Value& buttonData = componentLists[Serialization::BUTTON_COMPONENT];

			Button button;
			if (DeserializeComponent(buttonData, button)) {
				entityMgr.AddComponent<Button>(loadedEntity, button);
			}
		}

		// Deserialize SceneOwned Component
		if (componentLists.HasMember(Serialization::SCENE_OWNED_COMPONENT)) {
			const rapidjson::Value& sceneOwnedData = componentLists[Serialization::SCENE_OWNED_COMPONENT];

			SceneOwned sceneOwnedComp;
			if (DeserializeComponent(sceneOwnedData, sceneOwnedComp)) {
				entityMgr.AddComponent<SceneOwned>(loadedEntity, sceneOwnedComp);
			}
		}

		// Deserialize ParticleEmitter Component
		if (componentLists.HasMember(Serialization::PARTICLE_EMITTER_COMPONENT)) {
			const rapidjson::Value& particleEmitterData = componentLists[Serialization::PARTICLE_EMITTER_COMPONENT];
			ParticleEmitterComponent particleEmitter;
			if (DeserializeComponent(particleEmitterData, particleEmitter)) {
				entityMgr.AddComponent<ParticleEmitterComponent>(loadedEntity, particleEmitter);
			}
		}
	}

	void EntitySerializer::DeserializeTags(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData) {
		const rapidjson::Value& tagsList = entityData[Serialization::TAGS];

		// Deserialize Tags
		for (rapidjson::SizeType i = 0; i < tagsList.Size(); ++i) {
			if (tagsList[i].IsString()) {
				std::string tag = tagsList[i].GetString();
				if (tag == Serialization::PLAYER_TAG) {
					entityMgr.AddComponent<PlayerTag>(loadedEntity, PlayerTag{});
				}
				else if (tag == Serialization::PLATFORM_TAG) {
					entityMgr.AddComponent<PlatformTag>(loadedEntity, PlatformTag{});
				}
				else if (tag == Serialization::BACKGROUND_TAG) {
					entityMgr.AddComponent<BackgroundTag>(loadedEntity, BackgroundTag{});
				}
				else if (tag == Serialization::ENEMY_TAG) {
					entityMgr.AddComponent<EnemyTag>(loadedEntity, EnemyTag{});
				}
				else if (tag == Serialization::PERSISTENT_TAG) {
					entityMgr.AddComponent<PersistentTag>(loadedEntity, PersistentTag{});
				}
				else if (tag == Serialization::ACTIVE_TAG) {
					entityMgr.AddComponent<ActiveTag>(loadedEntity, ActiveTag{});
				}
				else if (tag == Serialization::GUI_TAG) {
					entityMgr.AddComponent<GUITag>(loadedEntity, GUITag{});
				}
			}
		}
	}

	Entity EntitySerializer::DeserializeEntity(EntityStorage& ecs, const rapidjson::Value& entityData) {
		EntityManager entityMgr(ecs);

		Entity loadedEntity = entityMgr.CreateEntity();

		DeserializeEntityToExisting(ecs, loadedEntity, entityData);

		return loadedEntity;
	}

	void EntitySerializer::DeserializeEntityToExisting(EntityStorage& ecs, Entity entityToOverwrite, const rapidjson::Value& entityData) {
		EntityManager entityMgr(ecs);

		/* Entity Properties Deserialization */
		EntityProperties& properties = entityMgr.GetProperties(entityToOverwrite);
		DeserializeEntityProperties(entityData, properties);

		/* Components Deserialization */
		if (entityData.HasMember(Serialization::COMPONENTS)) {
			if (entityData[Serialization::COMPONENTS].IsObject()) {
				DeserializeComponents(entityMgr, entityToOverwrite, entityData);
			}
			else {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Components data is not an object." << std::endl;
			}
		}

		/* Tags Deserialization */
		if (entityData.HasMember(Serialization::TAGS)) {
			if (entityData[Serialization::TAGS].IsArray()) {
				DeserializeTags(entityMgr, entityToOverwrite, entityData);
			}
			else {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Tags data is not a list." << std::endl;
			}
		}
	}

	void EntitySerializer::DeserializeEntityProperties(const rapidjson::Value& entityData, EntityProperties& out) {
		if (entityData.HasMember(Serialization::ENTITY_NAME) && entityData[Serialization::ENTITY_NAME].IsString()) {
			out.name = entityData[Serialization::ENTITY_NAME].GetString();
		}
		if (entityData.HasMember(Serialization::ENTITY_LAYER) && entityData[Serialization::ENTITY_LAYER].IsInt()) {
			out.layer = static_cast<CollisionLayer>(entityData[Serialization::ENTITY_LAYER].GetInt());
		}

		if (entityData.HasMember(Serialization::ENTITY_RENDER_LAYER) && entityData[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
			out.renderLayer = static_cast<RenderLayer>(entityData[Serialization::ENTITY_RENDER_LAYER].GetInt());
		}
		else {
			// need this or might break old saves
			out.renderLayer = RenderLayer::Custom;
		}

		if (entityData.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER) && entityData[Serialization::ENTITY_CUSTOM_RENDER_ORDER].IsInt()) {
			out.customRenderOrder = entityData[Serialization::ENTITY_CUSTOM_RENDER_ORDER].GetInt();
		}
		else {
			out.customRenderOrder = 0;
		}

		if (entityData.HasMember(Serialization::ENTITY_IS_ACTIVE) && entityData[Serialization::ENTITY_IS_ACTIVE].IsBool()) {
			out.isActive = entityData[Serialization::ENTITY_IS_ACTIVE].GetBool();
		}
	}

	bool EntitySerializer::DeserializeSceneData(EntityStorage& ecs, const std::string& filePath, std::string& sceneName, std::vector<Entity>& entitiesList) {
		try {
			Logger::Get().Info(DebugType::EntitySeralizer) << "Deserializing scene data from: " << filePath << std::endl;
			rapidjson::Document sceneData;
			if (!FileIOSystem::ReadFile(filePath, sceneData)) {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Unable to read file " << filePath << std::endl;
				return false;
			}
			if (!sceneData.HasMember(Serialization::SCENE_NAME) || !sceneData[Serialization::SCENE_NAME].IsString()) {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Missing or invalid '" << std::string(Serialization::SCENE_NAME) << "' field.\n";
				return false;
			}
			if (!sceneData.HasMember(Serialization::ENTITIES) || !sceneData[Serialization::ENTITIES].IsArray()) {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Missing or invalid '" << std::string(Serialization::ENTITIES) << "' array.\n";
				return false;
			}

			// Deserialize scene name and entities
			sceneName = sceneData[Serialization::SCENE_NAME].GetString();
			const rapidjson::Value& entitiesData = sceneData[Serialization::ENTITIES];
			if (entitiesData.IsArray()) {
				for (const rapidjson::Value& en : entitiesData.GetArray()) {
					entitiesList.push_back(DeserializeEntity(ecs, en));
				}
			}
			return true;
		}
		catch (...) {
			Logger::Get().Error(DebugType::EntitySeralizer) << "Failed to deserialize scene data from " << filePath << std::endl;
		}
		return false;
	}

#pragma region SPECIALIZED SERIALIZATION

	// Transform Component
	void EntitySerializer::SerializeComponent(const Transform& transform, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		// Position
		rapidjson::Value pos(rapidjson::kObjectType);
		pos.AddMember(rapidjson::StringRef(Serialization::VEC2_X), transform.position.x, alloc);
		pos.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), transform.position.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TRANSFORM_POSITION), pos, alloc);

		// Rotation
		rapidjson::Value rot(rapidjson::kObjectType);
		rot.AddMember(rapidjson::StringRef(Serialization::ROTATION_ANGLE), transform.rotation.angle, alloc);
		rot.AddMember(rapidjson::StringRef(Serialization::ROTATION_TEXTURE_ANGLE), transform.rotation.textureAngle, alloc);
		rot.AddMember(rapidjson::StringRef(Serialization::ROTATION_COLLIDER_ANGLE), transform.rotation.colliderAngle, alloc);

		rapidjson::Value textureMatrix(rapidjson::kArrayType);
		rapidjson::Value colliderMatrix(rapidjson::kArrayType);

		for (int i = 0; i < 9; ++i) {
			rapidjson::Value textureVal(transform.rotation.textureTransformMatrix.m[i]);
			textureMatrix.PushBack(textureVal, alloc);

			rapidjson::Value colliderVal(transform.rotation.colliderTransformMatrix.m[i]);
			colliderMatrix.PushBack(colliderVal, alloc);
		}

		rot.AddMember(rapidjson::StringRef(Serialization::ROTATION_TEXTURE_MTX), textureMatrix, alloc);
		rot.AddMember(rapidjson::StringRef(Serialization::ROTATION_COLLIDER_MTX), colliderMatrix, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TRANSFORM_ROTATION), rot, alloc);

		// Scale
		rapidjson::Value scale(rapidjson::kObjectType);
		scale.AddMember(rapidjson::StringRef(Serialization::VEC2_X), transform.scale.x, alloc);
		scale.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), transform.scale.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TRANSFORM_SCALE), scale, alloc);
	}

	// Spawnpoint Component
	void EntitySerializer::SerializeComponent(const SpawnPoint& spawnPt, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_X), spawnPt.Pos.x, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), spawnPt.Pos.y, alloc);
	}

	// Velocity Component
	void EntitySerializer::SerializeComponent(const Velocity& vel, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_X), vel.Vel.x, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), vel.Vel.y, alloc);
	}

	// Acceleration Component
	void EntitySerializer::SerializeComponent(const Acceleration& acc, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_X), acc.Accel.x, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), acc.Accel.y, alloc);
	}

	// Triangle Component
	void EntitySerializer::SerializeComponent(const Triangle& triangle, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value vertexA(rapidjson::kArrayType);
		vertexA.PushBack(triangle.a.x, alloc).PushBack(triangle.a.y, alloc);
		rapidjson::Value vertexB(rapidjson::kArrayType);
		vertexB.PushBack(triangle.b.x, alloc).PushBack(triangle.b.y, alloc);
		rapidjson::Value vertexC(rapidjson::kArrayType);
		vertexC.PushBack(triangle.c.x, alloc).PushBack(triangle.c.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VERTEX_A), vertexA, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VERTEX_B), vertexB, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VERTEX_C), vertexC, alloc);
		rapidjson::Value colorList(rapidjson::kArrayType);
		colorList.PushBack(triangle.color.r, alloc).PushBack(triangle.color.g, alloc);
		colorList.PushBack(triangle.color.b, alloc).PushBack(triangle.color.a, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::COLOR), colorList, alloc);
	}

	// RigidBody Component
	void EntitySerializer::SerializeComponent(const RigidBody& rb, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::MASS), rb.Mass, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::INVERSE_MASS), rb.InverseMass, alloc);
		rapidjson::Value force(rapidjson::kObjectType);
		force.AddMember(rapidjson::StringRef(Serialization::VEC2_X), rb.Force.x, alloc);
		force.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), rb.Force.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FORCE), force, alloc);
	}

	// AABBCollider Component
	void EntitySerializer::SerializeComponent(const AABBCollider& col, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::HALF_WIDTH), col.HalfSize.x, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HALF_HEIGHT), col.HalfSize.y, alloc);
	}

	// PlayerState Component
	void EntitySerializer::SerializeComponent(const PlayerState& state, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::PLAYER_IS_FACING_RIGHT), state.isFacingRight, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PLAYER_IS_GROUNDED), state.isGrounded, alloc);
	}

	// RotationFlag Component
	void EntitySerializer::SerializeComponent(const RotationFlag& rFlag, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::IS_ROTATING), rFlag.isRotating, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CURRENT_ROTATION), rFlag.currentRotation, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TARGET_ROTATION), rFlag.targetRotation, alloc);
	}

	// TrapPlatform Component
	void EntitySerializer::SerializeComponent(const TrapPlatformComponent& trap, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::IS_TRIGGERED), trap.isTriggered, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HAS_FALLEN), trap.hasFallen, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TRIGGER_DELAY), trap.triggerDelay, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CURRENT_DELAY), trap.currentDelay, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FALL_SPEED), trap.fallSpeed, alloc);
		rapidjson::Value originalPos(rapidjson::kObjectType);
		originalPos.AddMember(rapidjson::StringRef(Serialization::VEC2_X), trap.originalPosition.x, alloc);
		originalPos.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), trap.originalPosition.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ORIGINAL_POSITION), originalPos, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CURRENT_FALL_DISTANCE), trap.currentFallDistance, alloc);
	}

	// Texture Component
	void EntitySerializer::SerializeComponent(const TextureComponent& texComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::TEXTURE_KEY), rapidjson::StringRef(texComp.textureKey.c_str()), alloc);
		rapidjson::Value halfSize(rapidjson::kObjectType);
		halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_X), texComp.HalfSize.x, alloc);
		halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), texComp.HalfSize.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HALF_SIZE), halfSize, alloc);
	}

	// Audio Component
	void EntitySerializer::SerializeComponent(const AudioComponent& audioComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::SOUND_KEY), rapidjson::StringRef(audioComp.soundKey.c_str()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::SHOULD_LOOP), audioComp.loop, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_STREAMING), audioComp.stream, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VOLUME), audioComp.volume, alloc);
	}

	// Animation Component
	void EntitySerializer::SerializeComponent(const AnimationComponent& animComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value profileNameVal;
		// Handle empty string case
		const char* pName = animComp.profileName.empty() ? "" : animComp.profileName.c_str();
		profileNameVal.SetString(pName, static_cast<rapidjson::SizeType>(strlen(pName)), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ANIMATION_PROFILE), profileNameVal, alloc);

		outData.AddMember(rapidjson::StringRef(Serialization::CURRENT_STATE), static_cast<int>(animComp.currentState), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CURRENT_FRAME), animComp.currentFrame, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FRAME_DURATION), animComp.frameDuration, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::START_FRAME), animComp.startFrame, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FRAME_COUNT), animComp.frameCount, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ELAPSED_TIME), animComp.elapsedTime, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_PLAYING), animComp.isPlaying, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_LOOPING), animComp.isLooping, alloc);
	}

	// Text Component
	void EntitySerializer::SerializeComponent(const TextComponent& textComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::TEXT), rapidjson::StringRef(textComp.textToDraw.c_str()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FONT_KEY), rapidjson::StringRef(textComp.fontKey.c_str()), alloc);
		rapidjson::Value colorList(rapidjson::kArrayType);
		colorList.PushBack(textComp.color.r, alloc).PushBack(textComp.color.g, alloc);
		colorList.PushBack(textComp.color.b, alloc).PushBack(textComp.color.a, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FONT_COLOR), colorList, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FONT_SIZE), textComp.scale, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::MAX_WIDTH), textComp.maxWidth, alloc);
	}

	// Camera Component
	void EntitySerializer::SerializeComponent(const Camera& cameraComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value positionObj(rapidjson::kObjectType);
		positionObj.AddMember(rapidjson::StringRef(Serialization::VEC2_X), cameraComp.position.x, alloc);
		positionObj.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), cameraComp.position.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::POSITION), positionObj, alloc);
		rapidjson::Value sizeObj(rapidjson::kObjectType);
		sizeObj.AddMember(rapidjson::StringRef(Serialization::WIDTH), cameraComp.size.x, alloc);
		sizeObj.AddMember(rapidjson::StringRef(Serialization::HEIGHT), cameraComp.size.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::SIZE), sizeObj, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ZOOM), cameraComp.zoom, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TARGET_ENTITY), cameraComp.targetEntity, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::FOLLOW_SPEED), cameraComp.followSpeed, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_ACTIVE), cameraComp.isActive, alloc);
	}

	// NativeScripts Component
	void EntitySerializer::SerializeComponent(const NativeScriptsComponent& nativeScripts, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value scriptList(rapidjson::kArrayType);
		for (const NativeScriptsComponent::NativeScript& script : nativeScripts.scripts) {
			rapidjson::Value scriptObj(rapidjson::kObjectType);

			const char* typeCStr = script.scriptName ? script.scriptName : script.serializedTypeName.c_str();
			rapidjson::Value typeVal;
			typeVal.SetString(typeCStr, static_cast<rapidjson::SizeType>(std::char_traits<char>::length(typeCStr)), alloc);

			scriptObj.AddMember(rapidjson::StringRef(Serialization::SCRIPT_TYPE), typeVal, alloc);
			scriptObj.AddMember(rapidjson::StringRef(Serialization::SCRIPT_ENABLED), script.enabled, alloc);
			scriptObj.AddMember(rapidjson::StringRef(Serialization::SCRIPT_EXEC_IN_EDIT), script.executeInEditMode, alloc);

			scriptList.PushBack(scriptObj, alloc);
		}
		outData.AddMember(rapidjson::StringRef(Serialization::SCRIPTS), scriptList, alloc);
	}

	// BirdAI Component
	void EntitySerializer::SerializeComponent(const BirdAIComponent& birdAI, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::AI_STATE), static_cast<int>(birdAI.state), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_FACING_RIGHT), birdAI.isFacingRight, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::STATE_TIMER), birdAI.stateTimer, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::WAKEUP_RADIUS), birdAI.wakeUpRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::WAKEUP_TIME), birdAI.wakeUpTime, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::MOVE_SPEED), birdAI.moveSpeed, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CHASE_RADIUS), birdAI.chaseRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HOVER_RADIUS), birdAI.hoverRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HOVER_HEIGHT), birdAI.hoverHeight, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HOVER_SPEED), birdAI.hoverSpeed, alloc);

		outData.AddMember(rapidjson::StringRef(Serialization::ATTACK_RADIUS), birdAI.attackRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ATTACK_SPEED), birdAI.attackSpeed, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ATTACK_DURATION), birdAI.attackDuration, alloc);

		// ADD STUN DURATION:
		outData.AddMember(rapidjson::StringRef(Serialization::STUN_DURATION), birdAI.stunDuration, alloc);

		// Serialize attack target position
		rapidjson::Value attackTargetPos(rapidjson::kObjectType);
		attackTargetPos.AddMember(rapidjson::StringRef(Serialization::VEC2_X), birdAI.attackTargetPosition.x, alloc);
		attackTargetPos.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), birdAI.attackTargetPosition.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ATTACK_TARGET_POSITION), attackTargetPos, alloc);

		outData.AddMember(rapidjson::StringRef(Serialization::PATH_RECALCULATE_INTERVAL), birdAI.pathRecalculateInterval, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PATH_RECALCULATE_TIMER), birdAI.pathRecalculateTimer, alloc);
	}

	// SkeletonAI Component
	void EntitySerializer::SerializeComponent(const SkeletonAIComponent& skelAI, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::AI_STATE), static_cast<int>(skelAI.state), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::DETECTION_RADIUS), skelAI.detectionRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::MOVE_SPEED), skelAI.moveSpeed, alloc);
	}

	// Button Component
	void EntitySerializer::SerializeComponent(const Button& button, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::IS_HOVERED), button.isHovered, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_PRESSED), button.isPressed, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_RELEASED), button.isReleased, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ACTION), static_cast<int>(button.action), alloc);

		rapidjson::Value targetStrVal;
		targetStrVal.SetString(button.targetString.c_str(), static_cast<rapidjson::SizeType>(button.targetString.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TARGET_STRING), targetStrVal, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::TARGET_VALUE), button.targetValue, alloc);

		rapidjson::Value hoverSoundVal;
		hoverSoundVal.SetString(button.hoverSoundKey.c_str(), static_cast<rapidjson::SizeType>(button.hoverSoundKey.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HOVER_SOUND_KEY), hoverSoundVal, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::HOVER_SOUND_VOLUME), button.hoverSoundVolume, alloc);

		rapidjson::Value clickSoundVal;
		clickSoundVal.SetString(button.clickSoundKey.c_str(), static_cast<rapidjson::SizeType>(button.clickSoundKey.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CLICK_SOUND_KEY), clickSoundVal, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CLICK_SOUND_VOLUME), button.clickSoundVolume, alloc);
	
		rapidjson::Value defaultTexVal;
		defaultTexVal.SetString(button.defaultTextureKey.c_str(), static_cast<rapidjson::SizeType>(button.defaultTextureKey.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_DEFAULT_TEXTURE), defaultTexVal, alloc);

		rapidjson::Value hoverTexVal;
		hoverTexVal.SetString(button.hoverTextureKey.c_str(), static_cast<rapidjson::SizeType>(button.hoverTextureKey.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_HOVER_TEXTURE), hoverTexVal, alloc);

		rapidjson::Value pressedTexVal;
		pressedTexVal.SetString(button.pressedTextureKey.c_str(), static_cast<rapidjson::SizeType>(button.pressedTextureKey.length()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_PRESSED_TEXTURE), pressedTexVal, alloc);

		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_DEFAULT_ANIMATED), button.defaultIsAnimated, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_HOVER_ANIMATED), button.hoverIsAnimated, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::BUTTON_PRESSED_ANIMATED), button.pressedIsAnimated, alloc);
	}

	// SceneOwned Component
	void EntitySerializer::SerializeComponent(const SceneOwned& sceneOwnedComp, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::COMP_SCENE_NAME), rapidjson::StringRef(sceneOwnedComp.sceneName.c_str()), alloc);
	}

	// ParticleEmitter Component
	void EntitySerializer::SerializeComponent(const ParticleEmitterComponent& particleEmitter, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		outData.AddMember(rapidjson::StringRef(Serialization::IS_EMITTING), particleEmitter.isEmitting, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_PARTICLE_ANIMATED), particleEmitter.isParticleAnimated, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::IS_SCREEN_SPACE), particleEmitter.isScreenSpace, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_TEXTURE_KEY), rapidjson::StringRef(particleEmitter.particleTextureKey.c_str()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_LIFETIME), particleEmitter.particleLifetime, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_RATE), particleEmitter.emissionRate, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_INTERVAL), particleEmitter.emissionInterval, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_EFFECT_TYPE), static_cast<int>(particleEmitter.particleEffectType), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EFFECT_STRENGTH), particleEmitter.effectStrength, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_STYLE), static_cast<int>(particleEmitter.emissionStyle), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_SPEED), particleEmitter.particleSpeed, alloc);

		rapidjson::Value halfSize(rapidjson::kObjectType);
		halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.particleSize.x, alloc);
		halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.particleSize.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_SIZE), halfSize, alloc);

		outData.AddMember(rapidjson::StringRef(Serialization::IS_AFFECTED_BY_GRAVITY), particleEmitter.isAffectedByGravity, alloc);
		
		rapidjson::Value startVel(rapidjson::kObjectType);
		startVel.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.particleStartVelocity.x, alloc);
		startVel.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.particleStartVelocity.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_START_VELOCITY), startVel, alloc);

		rapidjson::Value accel(rapidjson::kObjectType);
		accel.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.particleAcceleration.x, alloc);
		accel.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.particleAcceleration.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::PARTICLE_ACCELERATION), accel, alloc);

		// Style Specific Data
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_RADIUS), particleEmitter.emissionRadius, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::CIRCLE_RADIUS), particleEmitter.circleRadius, alloc);

		rapidjson::Value lineStart(rapidjson::kObjectType);
		lineStart.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.lineStart.x, alloc);
		lineStart.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.lineStart.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::LINE_START), lineStart, alloc);

		rapidjson::Value lineEnd(rapidjson::kObjectType);
		lineEnd.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.lineEnd.x, alloc);
		lineEnd.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.lineEnd.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::LINE_END), lineEnd, alloc);

		rapidjson::Value viewMin(rapidjson::kObjectType);
		viewMin.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.viewportMin.x, alloc);
		viewMin.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.viewportMin.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VIEWPORT_MIN), viewMin, alloc);

		rapidjson::Value viewMax(rapidjson::kObjectType);
		viewMax.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.viewportMax.x, alloc);
		viewMax.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.viewportMax.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::VIEWPORT_MAX), viewMax, alloc);

		rapidjson::Value emitDir(rapidjson::kObjectType);
		emitDir.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.emissionDirection.x, alloc);
		emitDir.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.emissionDirection.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_DIRECTION), emitDir, alloc);

		rapidjson::Value emitOffset(rapidjson::kObjectType);
		emitOffset.AddMember(rapidjson::StringRef(Serialization::VEC2_X), particleEmitter.emissionOffset.x, alloc);
		emitOffset.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), particleEmitter.emissionOffset.y, alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::EMISSION_OFFSET), emitOffset, alloc);
	}
#pragma endregion

#pragma region SPECIALIZED DESERIALIZATION

	// Transform Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Transform& out) {
		if (!componentData.IsObject()) return false;

		// Position
		if (componentData.HasMember(Serialization::TRANSFORM_POSITION) && componentData[Serialization::TRANSFORM_POSITION].IsObject()) {
			const rapidjson::Value& positionData = componentData[Serialization::TRANSFORM_POSITION];
			if (positionData.HasMember(Serialization::VEC2_X) && positionData[Serialization::VEC2_X].IsFloat()) {
				out.position.x = positionData[Serialization::VEC2_X].GetFloat();
			}
			if (positionData.HasMember(Serialization::VEC2_Y) && positionData[Serialization::VEC2_Y].IsFloat()) {
				out.position.y = positionData[Serialization::VEC2_Y].GetFloat();
			}
		}

		// Rotation
		if (componentData.HasMember(Serialization::TRANSFORM_ROTATION) && componentData[Serialization::TRANSFORM_ROTATION].IsObject()) {
			const rapidjson::Value& rotationData = componentData[Serialization::TRANSFORM_ROTATION];
			if (rotationData.HasMember(Serialization::ROTATION_ANGLE) && rotationData[Serialization::ROTATION_ANGLE].IsFloat()) {
				out.rotation.angle = rotationData[Serialization::ROTATION_ANGLE].GetFloat();
			}
			if (rotationData.HasMember(Serialization::ROTATION_TEXTURE_ANGLE) && rotationData[Serialization::ROTATION_TEXTURE_ANGLE].IsFloat()) {
				out.rotation.textureAngle = rotationData[Serialization::ROTATION_TEXTURE_ANGLE].GetFloat();
			}
			if (rotationData.HasMember(Serialization::ROTATION_COLLIDER_ANGLE) && rotationData[Serialization::ROTATION_COLLIDER_ANGLE].IsFloat()) {
				out.rotation.colliderAngle = rotationData[Serialization::ROTATION_COLLIDER_ANGLE].GetFloat();
			}

			Matrix3x3 matrix;
			if (rotationData.HasMember(Serialization::ROTATION_TEXTURE_MTX) && rotationData[Serialization::ROTATION_TEXTURE_MTX].IsArray()) {
				rapidjson::GenericArray textureMtxArray = rotationData[Serialization::ROTATION_TEXTURE_MTX].GetArray();
				for (int i = 0; i < 9; ++i) {
					matrix.m[i] = textureMtxArray[i].GetFloat();
				}

				out.rotation.textureTransformMatrix = matrix;
			}
			if (rotationData.HasMember(Serialization::ROTATION_COLLIDER_MTX) && rotationData[Serialization::ROTATION_COLLIDER_MTX].IsArray()) {
				rapidjson::GenericArray colliderMtxArray = rotationData[Serialization::ROTATION_COLLIDER_MTX].GetArray();
				for (int i = 0; i < 9; ++i) {
					matrix.m[i] = colliderMtxArray[i].GetFloat();
				}

				out.rotation.colliderTransformMatrix = matrix;
			}
		}

		// Scale
		if (componentData.HasMember(Serialization::TRANSFORM_SCALE) && componentData[Serialization::TRANSFORM_SCALE].IsObject()) {
			const rapidjson::Value& scaleData = componentData[Serialization::TRANSFORM_SCALE];
			if (scaleData.HasMember(Serialization::VEC2_X) && scaleData[Serialization::VEC2_X].IsFloat()) {
				out.scale.x = scaleData[Serialization::VEC2_X].GetFloat();
			}
			if (scaleData.HasMember(Serialization::VEC2_Y) && scaleData[Serialization::VEC2_Y].IsFloat()) {
				out.scale.y = scaleData[Serialization::VEC2_Y].GetFloat();
			}
		}
		return true;
	}

	// SpawnPoint Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, SpawnPoint& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::VEC2_X) && componentData[Serialization::VEC2_X].IsFloat()) {
			out.Pos.x = componentData[Serialization::VEC2_X].GetFloat();
		}
		if (componentData.HasMember(Serialization::VEC2_Y) && componentData[Serialization::VEC2_Y].IsFloat()) {
			out.Pos.y = componentData[Serialization::VEC2_Y].GetFloat();
		}
		return true;
	}

	// Velocity Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Velocity& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::VEC2_X) && componentData[Serialization::VEC2_X].IsFloat()) {
			out.Vel.x = componentData[Serialization::VEC2_X].GetFloat();
		}
		if (componentData.HasMember(Serialization::VEC2_Y) && componentData[Serialization::VEC2_Y].IsFloat()) {
			out.Vel.y = componentData[Serialization::VEC2_Y].GetFloat();
		}
		return true;
	}

	// Acceleration Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Acceleration& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::VEC2_X) && componentData[Serialization::VEC2_X].IsFloat()) {
			out.Accel.x = componentData[Serialization::VEC2_X].GetFloat();
		}
		if (componentData.HasMember(Serialization::VEC2_Y) && componentData[Serialization::VEC2_Y].IsFloat()) {
			out.Accel.y = componentData[Serialization::VEC2_Y].GetFloat();
		}
		return true;
	}

	// Triangle Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Triangle& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::VERTEX_A) && componentData[Serialization::VERTEX_A].IsArray()) {
			const rapidjson::Value& vertexA = componentData[Serialization::VERTEX_A];
			out.a.x = vertexA[0].GetFloat();
			out.a.y = vertexA[1].GetFloat();
		}
		if (componentData.HasMember(Serialization::VERTEX_B) && componentData[Serialization::VERTEX_B].IsArray()) {
			const rapidjson::Value& vertexB = componentData[Serialization::VERTEX_B];
			out.b.x = vertexB[0].GetFloat();
			out.b.y = vertexB[1].GetFloat();
		}
		if (componentData.HasMember(Serialization::VERTEX_C) && componentData[Serialization::VERTEX_C].IsArray()) {
			const rapidjson::Value& vertexC = componentData[Serialization::VERTEX_C];
			out.c.x = vertexC[0].GetFloat();
			out.c.y = vertexC[1].GetFloat();
		}
		if (componentData.HasMember(Serialization::COLOR) && componentData[Serialization::COLOR].IsArray()) {
			const rapidjson::Value& colorArr = componentData[Serialization::COLOR];
			out.color.r = (float)colorArr[0].GetFloat();
			out.color.g = (float)colorArr[1].GetFloat();
			out.color.b = (float)colorArr[2].GetFloat();
			out.color.a = (float)colorArr[3].GetFloat();
		}
		return true;
	}

	// RigidBody Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, RigidBody& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::MASS) && componentData[Serialization::MASS].IsFloat()) {
			out.Mass = componentData[Serialization::MASS].GetFloat();
		}
		if (componentData.HasMember(Serialization::INVERSE_MASS) && componentData[Serialization::INVERSE_MASS].IsFloat()) {
			out.InverseMass = componentData[Serialization::INVERSE_MASS].GetFloat();
		}
		if (componentData.HasMember(Serialization::FORCE) && componentData[Serialization::FORCE].IsObject()) {
			const rapidjson::Value& forceData = componentData[Serialization::FORCE];
			if (forceData.HasMember(Serialization::VEC2_X) && forceData[Serialization::VEC2_X].IsFloat()) {
				out.Force.x = forceData[Serialization::VEC2_X].GetFloat();
			}
			if (forceData.HasMember(Serialization::VEC2_Y) && forceData[Serialization::VEC2_Y].IsFloat()) {
				out.Force.y = forceData[Serialization::VEC2_Y].GetFloat();
			}
		}
		return true;
	}

	// AABBCollider Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, AABBCollider& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::HALF_WIDTH) && componentData[Serialization::HALF_WIDTH].IsFloat()) {
			out.HalfSize.x = componentData[Serialization::HALF_WIDTH].GetFloat();
		}
		if (componentData.HasMember(Serialization::HALF_HEIGHT) && componentData[Serialization::HALF_HEIGHT].IsFloat()) {
			out.HalfSize.y = componentData[Serialization::HALF_HEIGHT].GetFloat();
		}
		return true;
	}

	// PlayerState Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, PlayerState& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::PLAYER_IS_FACING_RIGHT) && componentData[Serialization::PLAYER_IS_FACING_RIGHT].IsBool()) {
			out.isFacingRight = componentData[Serialization::PLAYER_IS_FACING_RIGHT].GetBool();
		}
		if (componentData.HasMember(Serialization::PLAYER_IS_GROUNDED) && componentData[Serialization::PLAYER_IS_GROUNDED].IsBool()) {
			out.isGrounded = componentData[Serialization::PLAYER_IS_GROUNDED].GetBool();
		}
		return true;
	}

	// RotationFlag Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, RotationFlag& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::IS_ROTATING) && componentData[Serialization::IS_ROTATING].IsBool()) {
			out.isRotating = componentData[Serialization::IS_ROTATING].GetBool();
		}
		if (componentData.HasMember(Serialization::CURRENT_ROTATION) && componentData[Serialization::CURRENT_ROTATION].IsFloat()) {
			out.currentRotation = componentData[Serialization::CURRENT_ROTATION].GetFloat();
		}
		if (componentData.HasMember(Serialization::TARGET_ROTATION) && componentData[Serialization::TARGET_ROTATION].IsFloat()) {
			out.targetRotation = componentData[Serialization::TARGET_ROTATION].GetFloat();
		}
		return true;
	}

	// TrapPlatform Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, TrapPlatformComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::IS_TRIGGERED) && componentData[Serialization::IS_TRIGGERED].IsBool()) {
			out.isTriggered = componentData[Serialization::IS_TRIGGERED].GetBool();
		}
		if (componentData.HasMember(Serialization::HAS_FALLEN) && componentData[Serialization::HAS_FALLEN].IsBool()) {
			out.hasFallen = componentData[Serialization::HAS_FALLEN].GetBool();
		}
		if (componentData.HasMember(Serialization::TRIGGER_DELAY) && componentData[Serialization::TRIGGER_DELAY].IsFloat()) {
			out.triggerDelay = componentData[Serialization::TRIGGER_DELAY].GetFloat();
		}
		if (componentData.HasMember(Serialization::CURRENT_DELAY) && componentData[Serialization::CURRENT_DELAY].IsFloat()) {
			out.currentDelay = componentData[Serialization::CURRENT_DELAY].GetFloat();
		}
		if (componentData.HasMember(Serialization::FALL_SPEED) && componentData[Serialization::FALL_SPEED].IsFloat()) {
			out.fallSpeed = componentData[Serialization::FALL_SPEED].GetFloat();
		}
		if (componentData.HasMember(Serialization::ORIGINAL_POSITION) && componentData[Serialization::ORIGINAL_POSITION].IsObject()) {
			const rapidjson::Value& originalPosData = componentData[Serialization::ORIGINAL_POSITION];
			if (originalPosData.HasMember(Serialization::VEC2_X) && originalPosData[Serialization::VEC2_X].IsFloat()) {
				out.originalPosition.x = originalPosData[Serialization::VEC2_X].GetFloat();
			}
			if (originalPosData.HasMember(Serialization::VEC2_Y) && originalPosData[Serialization::VEC2_Y].IsFloat()) {
				out.originalPosition.y = originalPosData[Serialization::VEC2_Y].GetFloat();
			}
		}
		if (componentData.HasMember(Serialization::CURRENT_FALL_DISTANCE) && componentData[Serialization::CURRENT_FALL_DISTANCE].IsFloat()) {
			out.currentFallDistance = componentData[Serialization::CURRENT_FALL_DISTANCE].GetFloat();
		}
		return true;
	}

	// Texture Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, TextureComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::TEXTURE_KEY) && componentData[Serialization::TEXTURE_KEY].IsString()) {
			out.textureKey = componentData[Serialization::TEXTURE_KEY].GetString();
		}
		if (componentData.HasMember(Serialization::HALF_SIZE) && componentData[Serialization::HALF_SIZE].IsObject()) {
			const rapidjson::Value& halfSizeData = componentData[Serialization::HALF_SIZE];
			if (halfSizeData.HasMember(Serialization::VEC2_X) && halfSizeData[Serialization::VEC2_X].IsFloat()) {
				out.HalfSize.x = halfSizeData[Serialization::VEC2_X].GetFloat();
			}
			if (halfSizeData.HasMember(Serialization::VEC2_Y) && halfSizeData[Serialization::VEC2_Y].IsFloat()) {
				out.HalfSize.y = halfSizeData[Serialization::VEC2_Y].GetFloat();
			}
		}
		return true;
	}

	// Audio Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, AudioComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::SOUND_KEY) && componentData[Serialization::SOUND_KEY].IsString()) {
			out.soundKey = componentData[Serialization::SOUND_KEY].GetString();
		}
		if (componentData.HasMember(Serialization::SHOULD_LOOP) && componentData[Serialization::SHOULD_LOOP].IsBool()) {
			out.loop = componentData[Serialization::SHOULD_LOOP].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_STREAMING) && componentData[Serialization::IS_STREAMING].IsBool()) {
			out.stream = componentData[Serialization::IS_STREAMING].GetBool();
		}
		if (componentData.HasMember(Serialization::VOLUME) && componentData[Serialization::VOLUME].IsFloat()) {
			out.volume = componentData[Serialization::VOLUME].GetFloat();
		}
		return true;
	}

	// Animation Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, AnimationComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::ANIMATION_PROFILE) && componentData[Serialization::ANIMATION_PROFILE].IsString()) {
			out.profileName = componentData[Serialization::ANIMATION_PROFILE].GetString();
		}

		if (componentData.HasMember(Serialization::CURRENT_STATE) && componentData[Serialization::CURRENT_STATE].IsInt()) {
			out.currentState = static_cast<AnimationState>(componentData[Serialization::CURRENT_STATE].GetInt());
		}
		if (componentData.HasMember(Serialization::CURRENT_FRAME) && componentData[Serialization::CURRENT_FRAME].IsInt()) {
			out.currentFrame = componentData[Serialization::CURRENT_FRAME].GetInt();
		}
		if (componentData.HasMember(Serialization::FRAME_DURATION) && componentData[Serialization::FRAME_DURATION].IsFloat()) {
			out.frameDuration = componentData[Serialization::FRAME_DURATION].GetFloat();
		}
		if (componentData.HasMember(Serialization::START_FRAME) && componentData[Serialization::START_FRAME].IsInt()) {
			out.startFrame = componentData[Serialization::START_FRAME].GetInt();
		}
		if (componentData.HasMember(Serialization::FRAME_COUNT) && componentData[Serialization::FRAME_COUNT].IsInt()) {
			out.frameCount = componentData[Serialization::FRAME_COUNT].GetInt();
		}
		if (componentData.HasMember(Serialization::ELAPSED_TIME) && componentData[Serialization::ELAPSED_TIME].IsFloat()) {
			out.elapsedTime = componentData[Serialization::ELAPSED_TIME].GetFloat();
		}
		if (componentData.HasMember(Serialization::IS_PLAYING) && componentData[Serialization::IS_PLAYING].IsBool()) {
			out.isPlaying = componentData[Serialization::IS_PLAYING].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_LOOPING) && componentData[Serialization::IS_LOOPING].IsBool()) {
			out.isLooping = componentData[Serialization::IS_LOOPING].GetBool();
		}
		return true;
	}

	// Text Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, TextComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::TEXT) && componentData[Serialization::TEXT].IsString()) {
			out.textToDraw = componentData[Serialization::TEXT].GetString();
		}
		if (componentData.HasMember(Serialization::FONT_KEY) && componentData[Serialization::FONT_KEY].IsString()) {
			out.fontKey = componentData[Serialization::FONT_KEY].GetString();
		}
		if (componentData.HasMember(Serialization::FONT_COLOR) && componentData[Serialization::FONT_COLOR].IsArray()) {
			const rapidjson::Value& colorArr = componentData[Serialization::FONT_COLOR];
			out.color.r = colorArr[0].GetFloat();
			out.color.g = colorArr[1].GetFloat();
			out.color.b = colorArr[2].GetFloat();
			out.color.a = colorArr[3].GetFloat();
		}
		if (componentData.HasMember(Serialization::FONT_SIZE) && componentData[Serialization::FONT_SIZE].IsFloat()) {
			out.scale = componentData[Serialization::FONT_SIZE].GetFloat();
		}
		if (componentData.HasMember(Serialization::MAX_WIDTH) && componentData[Serialization::MAX_WIDTH].IsFloat()) {
			out.maxWidth = componentData[Serialization::MAX_WIDTH].GetFloat();
		}
		return true;
	}

	// Camera Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Camera& out) {
		if (!componentData.IsObject()) return false;
		
		if (componentData.HasMember(Serialization::POSITION) && componentData[Serialization::POSITION].IsObject()) {
			const rapidjson::Value& positionData = componentData[Serialization::POSITION];
			if (positionData.HasMember(Serialization::VEC2_X) && positionData[Serialization::VEC2_X].IsFloat()) {
				out.position.x = positionData[Serialization::VEC2_X].GetFloat();
			}
			if (positionData.HasMember(Serialization::VEC2_Y) && positionData[Serialization::VEC2_Y].IsFloat()) {
				out.position.y = positionData[Serialization::VEC2_Y].GetFloat();
			}
		}
		if (componentData.HasMember(Serialization::SIZE) && componentData[Serialization::SIZE].IsObject()) {
			const rapidjson::Value& sizeData = componentData[Serialization::SIZE];
			if (sizeData.HasMember(Serialization::WIDTH) && sizeData[Serialization::WIDTH].IsFloat()) {
				out.size.x = sizeData[Serialization::WIDTH].GetFloat();
			}
			if (sizeData.HasMember(Serialization::HEIGHT) && sizeData[Serialization::HEIGHT].IsFloat()) {
				out.size.y = sizeData[Serialization::HEIGHT].GetFloat();
			}
		}
		if (componentData.HasMember(Serialization::ZOOM) && componentData[Serialization::ZOOM].IsFloat()) {
			out.zoom = componentData[Serialization::ZOOM].GetFloat();
		}
		if (componentData.HasMember(Serialization::TARGET_ENTITY) && componentData[Serialization::TARGET_ENTITY].IsUint64()) {
			out.targetEntity = (uint32_t)componentData[Serialization::TARGET_ENTITY].GetUint64();
		}
		if (componentData.HasMember(Serialization::FOLLOW_SPEED) && componentData[Serialization::FOLLOW_SPEED].IsFloat()) {
			out.followSpeed = componentData[Serialization::FOLLOW_SPEED].GetFloat();
		}
		if (componentData.HasMember(Serialization::IS_ACTIVE) && componentData[Serialization::IS_ACTIVE].IsBool()) {
			out.isActive = componentData[Serialization::IS_ACTIVE].GetBool();
		}
		return true;
	}

	// NativeScript Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, NativeScriptsComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::SCRIPTS) && componentData[Serialization::SCRIPTS].IsArray()) {
			const rapidjson::Value& scriptsArr = componentData[Serialization::SCRIPTS];

			for (const rapidjson::Value& scriptObj : scriptsArr.GetArray()) {
				if (!scriptObj.IsObject()) continue;
				std::string typeName;
				bool enabled = true;
				bool executeInEdit = false;

				if (scriptObj.HasMember(Serialization::SCRIPT_TYPE) && scriptObj[Serialization::SCRIPT_TYPE].IsString()) {
					typeName = scriptObj[Serialization::SCRIPT_TYPE].GetString();
				}
				if (scriptObj.HasMember(Serialization::SCRIPT_ENABLED) && scriptObj[Serialization::SCRIPT_ENABLED].IsBool()) {
					enabled = scriptObj[Serialization::SCRIPT_ENABLED].GetBool();
				}
				if (scriptObj.HasMember(Serialization::SCRIPT_EXEC_IN_EDIT) && scriptObj[Serialization::SCRIPT_EXEC_IN_EDIT].IsBool()) {
					executeInEdit = scriptObj[Serialization::SCRIPT_EXEC_IN_EDIT].GetBool();
				}

				out.scripts.emplace_back();
				NativeScriptsComponent::NativeScript& script = out.scripts.back();
				script.enabled = enabled;
				script.executeInEditMode = executeInEdit;

				if (!typeName.empty()) {
					const bool isGood = Framework::ScriptRegistry::Get().BindScriptByName(script, typeName);
					if (!isGood) {
						script.FlagMissing(typeName);
						Logger::Error(DebugType::EntitySeralizer) << "Unknown Script Type or Missing Script" << std::endl;
					}
				}
			}
		}
		return true;
	}

	// BirdAI Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, BirdAIComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::AI_STATE) && componentData[Serialization::AI_STATE].IsInt()) {
			out.state = static_cast<BirdState>(componentData[Serialization::AI_STATE].GetInt());
		}
		if (componentData.HasMember(Serialization::STATE_TIMER) && componentData[Serialization::STATE_TIMER].IsFloat()) {
			out.stateTimer = componentData[Serialization::STATE_TIMER].GetFloat();
		}
		if (componentData.HasMember(Serialization::WAKEUP_RADIUS) && componentData[Serialization::WAKEUP_RADIUS].IsFloat()) {
			out.wakeUpRadius = componentData[Serialization::WAKEUP_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::WAKEUP_TIME) && componentData[Serialization::WAKEUP_TIME].IsFloat()) {
			out.wakeUpTime = componentData[Serialization::WAKEUP_TIME].GetFloat();
		}
		if (componentData.HasMember(Serialization::MOVE_SPEED) && componentData[Serialization::MOVE_SPEED].IsFloat()) {
			out.moveSpeed = componentData[Serialization::MOVE_SPEED].GetFloat();
		}

		if (componentData.HasMember(Serialization::IS_FACING_RIGHT) && componentData[Serialization::IS_FACING_RIGHT].IsBool()) {
			out.isFacingRight = componentData[Serialization::IS_FACING_RIGHT].GetBool();
		}

		//CHASE_RADIUS:
		if (componentData.HasMember(Serialization::CHASE_RADIUS) && componentData[Serialization::CHASE_RADIUS].IsFloat()) {
			out.chaseRadius = componentData[Serialization::CHASE_RADIUS].GetFloat();
		}

		if (componentData.HasMember(Serialization::HOVER_RADIUS) && componentData[Serialization::HOVER_RADIUS].IsFloat()) {
			out.hoverRadius = componentData[Serialization::HOVER_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::HOVER_HEIGHT) && componentData[Serialization::HOVER_HEIGHT].IsFloat()) {
			out.hoverHeight = componentData[Serialization::HOVER_HEIGHT].GetFloat();
		}
		if (componentData.HasMember(Serialization::HOVER_SPEED) && componentData[Serialization::HOVER_SPEED].IsFloat()) {
			out.hoverSpeed = componentData[Serialization::HOVER_SPEED].GetFloat();
		}

		if (componentData.HasMember(Serialization::ATTACK_RADIUS) && componentData[Serialization::ATTACK_RADIUS].IsFloat()) {
			out.attackRadius = componentData[Serialization::ATTACK_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::ATTACK_SPEED) && componentData[Serialization::ATTACK_SPEED].IsFloat()) {
			out.attackSpeed = componentData[Serialization::ATTACK_SPEED].GetFloat();
		}
		if (componentData.HasMember(Serialization::ATTACK_DURATION) && componentData[Serialization::ATTACK_DURATION].IsFloat()) {
			out.attackDuration = componentData[Serialization::ATTACK_DURATION].GetFloat();
		}

		// STUN DURATION:
		if (componentData.HasMember(Serialization::STUN_DURATION) && componentData[Serialization::STUN_DURATION].IsFloat()) {
			out.stunDuration = componentData[Serialization::STUN_DURATION].GetFloat();
		}

		// Deserialize attack target position
		if (componentData.HasMember(Serialization::ATTACK_TARGET_POSITION) && componentData[Serialization::ATTACK_TARGET_POSITION].IsObject()) {
			const rapidjson::Value& attackTargetPosData = componentData[Serialization::ATTACK_TARGET_POSITION];
			if (attackTargetPosData.HasMember(Serialization::VEC2_X) && attackTargetPosData[Serialization::VEC2_X].IsFloat()) {
				out.attackTargetPosition.x = attackTargetPosData[Serialization::VEC2_X].GetFloat();
			}
			if (attackTargetPosData.HasMember(Serialization::VEC2_Y) && attackTargetPosData[Serialization::VEC2_Y].IsFloat()) {
				out.attackTargetPosition.y = attackTargetPosData[Serialization::VEC2_Y].GetFloat();
			}
		}

		if (componentData.HasMember(Serialization::PATH_RECALCULATE_INTERVAL) && componentData[Serialization::PATH_RECALCULATE_INTERVAL].IsFloat()) {
			out.pathRecalculateInterval = componentData[Serialization::PATH_RECALCULATE_INTERVAL].GetFloat();
		}
		if (componentData.HasMember(Serialization::PATH_RECALCULATE_TIMER) && componentData[Serialization::PATH_RECALCULATE_TIMER].IsFloat()) {
			out.pathRecalculateTimer = componentData[Serialization::PATH_RECALCULATE_TIMER].GetFloat();
		}
		return true;
	}

	// SkeletonAI Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, SkeletonAIComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::AI_STATE) && componentData[Serialization::AI_STATE].IsInt()) {
			out.state = static_cast<SkeletonState>(componentData[Serialization::AI_STATE].GetInt());
		}
		if (componentData.HasMember(Serialization::DETECTION_RADIUS) && componentData[Serialization::DETECTION_RADIUS].IsFloat()) {
			out.detectionRadius = componentData[Serialization::DETECTION_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::MOVE_SPEED) && componentData[Serialization::MOVE_SPEED].IsFloat()) {
			out.moveSpeed = componentData[Serialization::MOVE_SPEED].GetFloat();
		}
		return true;
	}

	// Button Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, Button& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::IS_HOVERED) && componentData[Serialization::IS_HOVERED].IsBool()) {
			out.isHovered = componentData[Serialization::IS_HOVERED].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_PRESSED) && componentData[Serialization::IS_PRESSED].IsBool()) {
			out.isPressed = componentData[Serialization::IS_PRESSED].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_RELEASED) && componentData[Serialization::IS_RELEASED].IsBool()) {
			out.isReleased = componentData[Serialization::IS_RELEASED].GetBool();
		}
		if (componentData.HasMember(Serialization::ACTION) && componentData[Serialization::ACTION].IsInt()) {
			out.action = static_cast<Framework::ButtonAction>(componentData[Serialization::ACTION].GetInt());
		}
		if (componentData.HasMember(Serialization::TARGET_STRING) && componentData[Serialization::TARGET_STRING].IsString()) {
			out.targetString = componentData[Serialization::TARGET_STRING].GetString();
		}
		if (componentData.HasMember(Serialization::TARGET_VALUE) && componentData[Serialization::TARGET_VALUE].IsFloat()) {
			out.targetValue = componentData[Serialization::TARGET_VALUE].GetFloat();
		}

		if (componentData.HasMember(Serialization::HOVER_SOUND_KEY) && componentData[Serialization::HOVER_SOUND_KEY].IsString()) {
			out.hoverSoundKey = componentData[Serialization::HOVER_SOUND_KEY].GetString();
		}
		else {
			out.hoverSoundKey = ""; // Default for old save files
		}

		if (componentData.HasMember(Serialization::HOVER_SOUND_VOLUME) && componentData[Serialization::HOVER_SOUND_VOLUME].IsFloat()) {
			out.hoverSoundVolume = componentData[Serialization::HOVER_SOUND_VOLUME].GetFloat();
		}
		else {
			out.hoverSoundVolume = 0.5f; // Default for old save files
		}

		if (componentData.HasMember(Serialization::CLICK_SOUND_KEY) && componentData[Serialization::CLICK_SOUND_KEY].IsString()) {
			out.clickSoundKey = componentData[Serialization::CLICK_SOUND_KEY].GetString();
		}
		else {
			out.clickSoundKey = ""; // Default for old save files
		}

		if (componentData.HasMember(Serialization::CLICK_SOUND_VOLUME) && componentData[Serialization::CLICK_SOUND_VOLUME].IsFloat()) {
			out.clickSoundVolume = componentData[Serialization::CLICK_SOUND_VOLUME].GetFloat();
		}
		else {
			out.clickSoundVolume = 0.5f; // Default for old save files
		}

		if (componentData.HasMember(Serialization::BUTTON_DEFAULT_TEXTURE) &&
			componentData[Serialization::BUTTON_DEFAULT_TEXTURE].IsString()) {
			out.defaultTextureKey = componentData[Serialization::BUTTON_DEFAULT_TEXTURE].GetString();
		}

		if (componentData.HasMember(Serialization::BUTTON_HOVER_TEXTURE) &&
			componentData[Serialization::BUTTON_HOVER_TEXTURE].IsString()) {
			out.hoverTextureKey = componentData[Serialization::BUTTON_HOVER_TEXTURE].GetString();
		}

		if (componentData.HasMember(Serialization::BUTTON_PRESSED_TEXTURE) &&
			componentData[Serialization::BUTTON_PRESSED_TEXTURE].IsString()) {
			out.pressedTextureKey = componentData[Serialization::BUTTON_PRESSED_TEXTURE].GetString();
		}

		if (componentData.HasMember(Serialization::BUTTON_DEFAULT_ANIMATED) &&
			componentData[Serialization::BUTTON_DEFAULT_ANIMATED].IsBool()) {
			out.defaultIsAnimated = componentData[Serialization::BUTTON_DEFAULT_ANIMATED].GetBool();
		}

		if (componentData.HasMember(Serialization::BUTTON_HOVER_ANIMATED) &&
			componentData[Serialization::BUTTON_HOVER_ANIMATED].IsBool()) {
			out.hoverIsAnimated = componentData[Serialization::BUTTON_HOVER_ANIMATED].GetBool();
		}

		if (componentData.HasMember(Serialization::BUTTON_PRESSED_ANIMATED) &&
			componentData[Serialization::BUTTON_PRESSED_ANIMATED].IsBool()) {
			out.pressedIsAnimated = componentData[Serialization::BUTTON_PRESSED_ANIMATED].GetBool();
		}

		return true;
	}

	// SceneOwned Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, SceneOwned& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::COMP_SCENE_NAME) && componentData[Serialization::COMP_SCENE_NAME].IsString()) {
			out.sceneName = componentData[Serialization::COMP_SCENE_NAME].GetString();
		}
		return true;
	}

	// ParticleEmitter Component
	bool EntitySerializer::DeserializeComponent(const rapidjson::Value& componentData, ParticleEmitterComponent& out) {
		if (!componentData.IsObject()) return false;

		if (componentData.HasMember(Serialization::IS_EMITTING) && componentData[Serialization::IS_EMITTING].IsBool()) {
			out.isEmitting = componentData[Serialization::IS_EMITTING].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_PARTICLE_ANIMATED) && componentData[Serialization::IS_PARTICLE_ANIMATED].IsBool()) {
			out.isParticleAnimated = componentData[Serialization::IS_PARTICLE_ANIMATED].GetBool();
		}
		if (componentData.HasMember(Serialization::IS_SCREEN_SPACE) && componentData[Serialization::IS_SCREEN_SPACE].IsBool()) {
			out.isScreenSpace = componentData[Serialization::IS_SCREEN_SPACE].GetBool();
		}

		if (componentData.HasMember(Serialization::PARTICLE_TEXTURE_KEY) && componentData[Serialization::PARTICLE_TEXTURE_KEY].IsString()) {
			out.particleTextureKey = componentData[Serialization::PARTICLE_TEXTURE_KEY].GetString();
		}
		if (componentData.HasMember(Serialization::PARTICLE_LIFETIME) && componentData[Serialization::PARTICLE_LIFETIME].IsFloat()) {
			out.particleLifetime = componentData[Serialization::PARTICLE_LIFETIME].GetFloat();
		}
		if (componentData.HasMember(Serialization::EMISSION_RATE) && componentData[Serialization::EMISSION_RATE].IsFloat()) {
			out.emissionRate = componentData[Serialization::EMISSION_RATE].GetFloat();
		}
		if (componentData.HasMember(Serialization::EMISSION_INTERVAL) && componentData[Serialization::EMISSION_INTERVAL].IsFloat()) {
			out.emissionInterval = componentData[Serialization::EMISSION_INTERVAL].GetFloat();
		}

		// Enums
		if (componentData.HasMember(Serialization::PARTICLE_EFFECT_TYPE) && componentData[Serialization::PARTICLE_EFFECT_TYPE].IsInt()) {
			out.particleEffectType = static_cast<EffectType>(componentData[Serialization::PARTICLE_EFFECT_TYPE].GetInt());
		}

		if (componentData.HasMember(Serialization::EFFECT_STRENGTH) && componentData[Serialization::EFFECT_STRENGTH].IsFloat()) {
			out.effectStrength = componentData[Serialization::EFFECT_STRENGTH].GetFloat();
		}
		else {
			out.effectStrength = 1.0f; // Default for old save files
		}

		if (componentData.HasMember(Serialization::EMISSION_STYLE) && componentData[Serialization::EMISSION_STYLE].IsInt()) {
			out.emissionStyle = static_cast<EmissionStyle>(componentData[Serialization::EMISSION_STYLE].GetInt());
		}

		if (componentData.HasMember(Serialization::PARTICLE_SPEED) && componentData[Serialization::PARTICLE_SPEED].IsFloat()) {
			out.particleSpeed = componentData[Serialization::PARTICLE_SPEED].GetFloat();
		}

		// Size
		if (componentData.HasMember(Serialization::PARTICLE_SIZE) && componentData[Serialization::PARTICLE_SIZE].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::PARTICLE_SIZE];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) {
				out.particleSize.x = vecData[Serialization::VEC2_X].GetFloat();
			}
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) {
				out.particleSize.y = vecData[Serialization::VEC2_Y].GetFloat();
			}
		}

		// Physics
		if (componentData.HasMember(Serialization::IS_AFFECTED_BY_GRAVITY) && componentData[Serialization::IS_AFFECTED_BY_GRAVITY].IsBool()) {
			out.isAffectedByGravity = componentData[Serialization::IS_AFFECTED_BY_GRAVITY].GetBool();
		}

		if (componentData.HasMember(Serialization::PARTICLE_START_VELOCITY) && componentData[Serialization::PARTICLE_START_VELOCITY].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::PARTICLE_START_VELOCITY];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.particleStartVelocity.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.particleStartVelocity.y = vecData[Serialization::VEC2_Y].GetFloat();
		}
		if (componentData.HasMember(Serialization::PARTICLE_ACCELERATION) && componentData[Serialization::PARTICLE_ACCELERATION].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::PARTICLE_ACCELERATION];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.particleAcceleration.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.particleAcceleration.y = vecData[Serialization::VEC2_Y].GetFloat();
		}

		// Style Params
		if (componentData.HasMember(Serialization::EMISSION_RADIUS) && componentData[Serialization::EMISSION_RADIUS].IsFloat()) {
			out.emissionRadius = componentData[Serialization::EMISSION_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::CIRCLE_RADIUS) && componentData[Serialization::CIRCLE_RADIUS].IsFloat()) {
			out.circleRadius = componentData[Serialization::CIRCLE_RADIUS].GetFloat();
		}
		if (componentData.HasMember(Serialization::LINE_START) && componentData[Serialization::LINE_START].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::LINE_START];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.lineStart.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.lineStart.y = vecData[Serialization::VEC2_Y].GetFloat();
		}
		if (componentData.HasMember(Serialization::LINE_END) && componentData[Serialization::LINE_END].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::LINE_END];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.lineEnd.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.lineEnd.y = vecData[Serialization::VEC2_Y].GetFloat();
		}
		if (componentData.HasMember(Serialization::VIEWPORT_MIN) && componentData[Serialization::VIEWPORT_MIN].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::VIEWPORT_MIN];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.viewportMin.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.viewportMin.y = vecData[Serialization::VEC2_Y].GetFloat();
		}
		if (componentData.HasMember(Serialization::VIEWPORT_MAX) && componentData[Serialization::VIEWPORT_MAX].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::VIEWPORT_MAX];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.viewportMax.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.viewportMax.y = vecData[Serialization::VEC2_Y].GetFloat();
		}
		if (componentData.HasMember(Serialization::EMISSION_DIRECTION) && componentData[Serialization::EMISSION_DIRECTION].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::EMISSION_DIRECTION];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) out.emissionDirection.x = vecData[Serialization::VEC2_X].GetFloat();
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) out.emissionDirection.y = vecData[Serialization::VEC2_Y].GetFloat();
		}

		// Emission Offset
		if (componentData.HasMember(Serialization::EMISSION_OFFSET) && componentData[Serialization::EMISSION_OFFSET].IsObject()) {
			const rapidjson::Value& vecData = componentData[Serialization::EMISSION_OFFSET];
			if (vecData.HasMember(Serialization::VEC2_X) && vecData[Serialization::VEC2_X].IsFloat()) {
				out.emissionOffset.x = vecData[Serialization::VEC2_X].GetFloat();
			}
			if (vecData.HasMember(Serialization::VEC2_Y) && vecData[Serialization::VEC2_Y].IsFloat()) {
				out.emissionOffset.y = vecData[Serialization::VEC2_Y].GetFloat();
			}
		}
		// If the field doesn't exist (old save files), default to {0, 0}
		else {
			out.emissionOffset = { 0.0f, 0.0f };
		}

		return true;
	}

#pragma endregion

}