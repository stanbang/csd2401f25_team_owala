#include "EntitySerializer.h"
#include "SerializationKeys.h"
#include "EntityManager.h"
#include "FileUtility.h"
#include "Logger.h"

// Internal Helpers - Will move this and the Editor's version to a global utils file later.
namespace {
	using namespace Framework;

	static void WriteVector2D(rapidjson::Value& parent, const char* name, const Vector2D& vec, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("x", vec.x, alloc);
		obj.AddMember("y", vec.y, alloc);
		parent.AddMember(rapidjson::StringRef(name), obj, alloc);
	}
}

namespace Framework {
	void EntitySerializer::SerializeEntityInternal(Entity entity, EntityStorage& ecs, rapidjson::Value& outData, rapidjson::Document::AllocatorType& alloc) {
		EntityManager entityMgr(ecs);

		/* Entity Properties Serialization */
		EntityProperties& properties = entityMgr.GetProperties(entity);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_NAME), rapidjson::StringRef(properties.name.c_str()), alloc);
		outData.AddMember(rapidjson::StringRef(Serialization::ENTITY_LAYER), static_cast<int>(properties.layer), alloc);

		rapidjson::Value componentList(rapidjson::kObjectType);

		/* Components Serialization */
		// Serialize Position Component
		if (entityMgr.HasComponent(entity, ComponentType::PositionType)) {
			Position& pos = entityMgr.GetComponent<Position>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_X), pos.Pos.x, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), pos.Pos.y, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::POSITION), value, alloc);
		}

		// Serialize Velocity Component
		if (entityMgr.HasComponent(entity, ComponentType::VelocityType)) {
			Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_X), vel.Vel.x, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), vel.Vel.y, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::VELOCITY_COMPONENT), value, alloc);
		}

		// Serialize Acceleration Component
		if (entityMgr.HasComponent(entity, ComponentType::AccelerationType)) {
			Acceleration& acc = entityMgr.GetComponent<Acceleration>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_X), acc.Accel.x, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), acc.Accel.y, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ACCELERATION_COMPONENT), value, alloc);
		}

		// Serialize Triangle Component
		if (entityMgr.HasComponent(entity, ComponentType::TriangleType)) {
			Triangle& triangle = entityMgr.GetComponent<Triangle>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			rapidjson::Value vertexA(rapidjson::kArrayType);
			vertexA.PushBack(triangle.a.x, alloc).PushBack(triangle.a.y, alloc);
			rapidjson::Value vertexB(rapidjson::kArrayType);
			vertexB.PushBack(triangle.b.x, alloc).PushBack(triangle.b.y, alloc);
			rapidjson::Value vertexC(rapidjson::kArrayType);
			vertexC.PushBack(triangle.c.x, alloc).PushBack(triangle.c.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VERTEX_A), vertexA, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VERTEX_B), vertexB, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VERTEX_C), vertexC, alloc);
			rapidjson::Value colorList(rapidjson::kArrayType);
			colorList.PushBack(triangle.color.r, alloc).PushBack(triangle.color.g, alloc);
			colorList.PushBack(triangle.color.b, alloc).PushBack(triangle.color.a, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::COLOR), colorList, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TRIANGLE_COMPONENT), value, alloc);
		}

		// Serialize RigidBody Component
		if (entityMgr.HasComponent(entity, ComponentType::RigidBodyType)) {
			RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::MASS), rb.Mass, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::INVERSE_MASS), rb.InverseMass, alloc);
			rapidjson::Value force(rapidjson::kObjectType);
			force.AddMember(rapidjson::StringRef(Serialization::VEC2_X), rb.Force.x, alloc);
			force.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), rb.Force.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FORCE), force, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::RIGIDBODY_COMPONENT), value, alloc);
		}

		// Serialize AABBCollider Component
		if (entityMgr.HasComponent(entity, ComponentType::AABBColliderType)) {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::HALF_WIDTH), col.HalfSize.x, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::HALF_HEIGHT), col.HalfSize.y, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::AABB_COLLIDER_COMPONENT), value, alloc);
		}

		// Serialize PlayerState Component
		if (entityMgr.HasComponent(entity, ComponentType::PlayerStateType)) {
			PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			componentList.AddMember(rapidjson::StringRef(Serialization::PLAYER_STATE_COMPONENT), value, alloc);
		}

		// Serialize RotationFlag Component
		if (entityMgr.HasComponent(entity, ComponentType::RotationFlagType)) {
			RotationFlag& rFlag = entityMgr.GetComponent<RotationFlag>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::IS_ROTATING), rFlag.isRotating, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_ROTATION), rFlag.currentRotation, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::TARGET_ROTATION), rFlag.targetRotation, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ROTATIONFLAG_COMPONENT), value, alloc);
		}

		// Serialize TrapPlatform Component
		if (entityMgr.HasComponent(entity, ComponentType::TrapPlatformComponentType)) {
			TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::IS_TRIGGERED), trap.isTriggered, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::HAS_FALLEN), trap.hasFallen, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::TRIGGER_DELAY), trap.triggerDelay, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_DELAY), trap.currentDelay, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FALL_SPEED), trap.fallSpeed, alloc);
			rapidjson::Value originalPos(rapidjson::kObjectType);
			originalPos.AddMember(rapidjson::StringRef(Serialization::VEC2_X), trap.originalPosition.x, alloc);
			originalPos.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), trap.originalPosition.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::ORIGINAL_POSITION), originalPos, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_FALL_DISTANCE), trap.currentFallDistance, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TRAP_PLATFORM_COMPONENT), value, alloc);
		}

		// Serialize ScalingPlatform Component
		if (entityMgr.HasComponent(entity, ComponentType::ScalingPlatformComponentType)) {
			ScalingPlatformComponent& scalingPlatform = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			rapidjson::Value baseHalfSize(rapidjson::kObjectType);
			baseHalfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_X), scalingPlatform.baseHalfSize.x, alloc);
			baseHalfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), scalingPlatform.baseHalfSize.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::BASE_HALFSIZE), baseHalfSize, alloc);
			rapidjson::Value currentScale(rapidjson::kObjectType);
			currentScale.AddMember(rapidjson::StringRef(Serialization::VEC2_X), scalingPlatform.currentScaleX, alloc);
			currentScale.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), scalingPlatform.currentScaleY, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_SCALE), currentScale, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::SCALE_SPEED), scalingPlatform.scaleSpeed, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::MIN_SCALE), scalingPlatform.minScale, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::MAX_SCALE), scalingPlatform.maxScale, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::ELAPSED_TIME), scalingPlatform.time, alloc);
			rapidjson::Value transformMtx(rapidjson::kArrayType);
			for (int i = 0; i < 9; ++i) {
				transformMtx.PushBack(scalingPlatform.transformMatrix.m[i], alloc);
			}
			value.AddMember(rapidjson::StringRef(Serialization::TRANSFORM_MATRIX), transformMtx, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::SCALING_PLATFORM_COMPONENT), value, alloc);
		}

		// Serialize TextureComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextureComponentType)) {
			TextureComponent& texComp = entityMgr.GetComponent<TextureComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::TEXTURE_KEY), rapidjson::StringRef(texComp.textureKey.c_str()), alloc);
			rapidjson::Value halfSize(rapidjson::kObjectType);
			halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_X), texComp.HalfSize.x, alloc);
			halfSize.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), texComp.HalfSize.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::HALF_SIZE), halfSize, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TEXTURE_COMPONENT), value, alloc);
		}

		// Serialize AudioComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AudioComponentType)) {
			AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::SOUND_KEY), rapidjson::StringRef(audioComp.soundKey.c_str()), alloc);
			value.AddMember(rapidjson::StringRef(Serialization::SHOULD_LOOP), audioComp.loop, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::IS_STREAMING), audioComp.stream, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::VOLUME), audioComp.volume, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::AUDIO_COMPONENT), value, alloc);
		}

		// Serialize AnimationComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AnimationComponentType)) {
			AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_STATE), static_cast<int>(animComp.currentState), alloc);
			value.AddMember(rapidjson::StringRef(Serialization::CURRENT_FRAME), animComp.currentFrame, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FRAME_DURATION), animComp.frameDuration, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::START_FRAME), animComp.startFrame, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FRAME_COUNT), animComp.frameCount, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::ELAPSED_TIME), animComp.elapsedTime, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::IS_PLAYING), animComp.isPlaying, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::IS_LOOPING), animComp.isLooping, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::ANIMATION_COMPONENT), value, alloc);
		}

		// Serialize TextComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextComponentType)) {
			TextComponent& textComp = entityMgr.GetComponent<TextComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::TEXT), rapidjson::StringRef(textComp.textToDraw.c_str()), alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FONT_KEY), rapidjson::StringRef(textComp.fontKey.c_str()), alloc);
			rapidjson::Value colorList(rapidjson::kArrayType);
			colorList.PushBack(textComp.color.r, alloc).PushBack(textComp.color.g, alloc);
			colorList.PushBack(textComp.color.b, alloc).PushBack(textComp.color.a, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FONT_COLOR), colorList, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FONT_SIZE), textComp.scale, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::TEXT_COMPONENT), value, alloc);
		}

		// Serialize Camera Component
		if (entityMgr.HasComponent(entity, ComponentType::CameraType)) {
			Camera& cameraComp = entityMgr.GetComponent<Camera>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			rapidjson::Value positionObj(rapidjson::kObjectType);
			positionObj.AddMember(rapidjson::StringRef(Serialization::VEC2_X), cameraComp.position.x, alloc);
			positionObj.AddMember(rapidjson::StringRef(Serialization::VEC2_Y), cameraComp.position.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::POSITION), positionObj, alloc);
			rapidjson::Value sizeObj(rapidjson::kObjectType);
			sizeObj.AddMember(rapidjson::StringRef(Serialization::WIDTH), cameraComp.size.x, alloc);
			sizeObj.AddMember(rapidjson::StringRef(Serialization::HEIGHT), cameraComp.size.y, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::SIZE), sizeObj, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::ZOOM), cameraComp.zoom, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::TARGET_ENTITY), cameraComp.targetEntity, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::FOLLOW_SPEED), cameraComp.followSpeed, alloc);
			value.AddMember(rapidjson::StringRef(Serialization::IS_ACTIVE), cameraComp.isActive, alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::CAMERA_COMPONENT), value, alloc);
		}

		if (entityMgr.HasComponent(entity, ComponentType::SceneOwnedType)) {
			SceneOwned& sceneOwnedComp = entityMgr.GetComponent<SceneOwned>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember(rapidjson::StringRef(Serialization::COMP_SCENE_NAME), rapidjson::StringRef(sceneOwnedComp.sceneName.c_str()), alloc);
			componentList.AddMember(rapidjson::StringRef(Serialization::SCENE_OWNED_COMPONENT), value, alloc);
		}

		outData.AddMember(rapidjson::StringRef(Serialization::COMPONENTS), componentList, alloc);

		/* Tags Serialization */
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

		outData.AddMember(rapidjson::StringRef(Serialization::TAGS), tagsList, alloc);
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

	void EntitySerializer::DeserializeComponents(EntityManager& entityMgr, Entity& loadedEntity, const rapidjson::Value& entityData) {
		const rapidjson::Value& componentLists = entityData[Serialization::COMPONENTS];

		// Deserialize Position Component
		if (componentLists.HasMember(Serialization::POSITION)) {
			const rapidjson::Value& pos = componentLists[Serialization::POSITION];

			Position position;
			if (pos.HasMember(Serialization::VEC2_X) && pos[Serialization::VEC2_X].IsFloat()) {
				position.Pos.x = pos[Serialization::VEC2_X].GetFloat();
			}
			if (pos.HasMember(Serialization::VEC2_Y) && pos[Serialization::VEC2_Y].IsFloat()) {
				position.Pos.y = pos[Serialization::VEC2_Y].GetFloat();
			}
			entityMgr.AddComponent<Position>(loadedEntity, position);
		}

		// Deserialize Velocity Component
		if (componentLists.HasMember(Serialization::VELOCITY_COMPONENT)) {
			const rapidjson::Value& vel = componentLists[Serialization::VELOCITY_COMPONENT];

			Velocity velocity;
			if (vel.HasMember(Serialization::VEC2_X) && vel[Serialization::VEC2_X].IsFloat()) {
				velocity.Vel.x = vel[Serialization::VEC2_X].GetFloat();
			}
			if (vel.HasMember(Serialization::VEC2_Y) && vel[Serialization::VEC2_Y].IsFloat()) {
				velocity.Vel.y = vel[Serialization::VEC2_Y].GetFloat();
			}
			entityMgr.AddComponent<Velocity>(loadedEntity, velocity);
		}

		// Deserialize Acceleration Component
		if (componentLists.HasMember(Serialization::ACCELERATION_COMPONENT)) {
			const rapidjson::Value& acc = componentLists[Serialization::ACCELERATION_COMPONENT];

			Acceleration acceleration;
			if (acc.HasMember(Serialization::VEC2_X) && acc[Serialization::VEC2_X].IsFloat()) {
				acceleration.Accel.x = acc[Serialization::VEC2_X].GetFloat();
			}
			if (acc.HasMember(Serialization::VEC2_Y) && acc[Serialization::VEC2_Y].IsFloat()) {
				acceleration.Accel.y = acc[Serialization::VEC2_Y].GetFloat();
			}
			entityMgr.AddComponent<Acceleration>(loadedEntity, acceleration);
		}

		// Deserialize Triangle Component
		if (componentLists.HasMember(Serialization::TRIANGLE_COMPONENT)) {
			const rapidjson::Value& triangleData = componentLists[Serialization::TRIANGLE_COMPONENT];

			Triangle triangle;
			if (triangleData.HasMember(Serialization::VERTEX_A) && triangleData[Serialization::VERTEX_A].IsArray()) {
				const rapidjson::Value& vertexA = triangleData[Serialization::VERTEX_A];
				triangle.a.x = vertexA[0].GetFloat();
				triangle.a.y = vertexA[1].GetFloat();
			}
			if (triangleData.HasMember(Serialization::VERTEX_B) && triangleData[Serialization::VERTEX_B].IsArray()) {
				const rapidjson::Value& vertexB = triangleData[Serialization::VERTEX_B];
				triangle.b.x = vertexB[0].GetFloat();
				triangle.b.y = vertexB[1].GetFloat();
			}
			if (triangleData.HasMember(Serialization::VERTEX_C) && triangleData[Serialization::VERTEX_C].IsArray()) {
				const rapidjson::Value& vertexC = triangleData[Serialization::VERTEX_C];
				triangle.c.x = vertexC[0].GetFloat();
				triangle.c.y = vertexC[1].GetFloat();
			}
			if (triangleData.HasMember(Serialization::COLOR) && triangleData[Serialization::COLOR].IsArray()) {
				const rapidjson::Value& colorArr = triangleData[Serialization::COLOR];
				triangle.color.r = (float)colorArr[0].GetUint();
				triangle.color.g = (float)colorArr[1].GetUint();
				triangle.color.b = (float)colorArr[2].GetUint();
				triangle.color.a = (float)colorArr[3].GetUint();
			}
			entityMgr.AddComponent<Triangle>(loadedEntity, triangle);
		}

		// Deserialize RigidBody Component
		if (componentLists.HasMember(Serialization::RIGIDBODY_COMPONENT)) {
			const rapidjson::Value& rbData = componentLists[Serialization::RIGIDBODY_COMPONENT];

			RigidBody rigidBody;
			if (rbData.HasMember(Serialization::MASS) && rbData[Serialization::MASS].IsFloat()) {
				rigidBody.Mass = rbData[Serialization::MASS].GetFloat();
			}
			if (rbData.HasMember(Serialization::INVERSE_MASS) && rbData[Serialization::INVERSE_MASS].IsFloat()) {
				rigidBody.InverseMass = rbData[Serialization::INVERSE_MASS].GetFloat();
			}
			if (rbData.HasMember(Serialization::FORCE) && rbData[Serialization::FORCE].IsObject()) {
				const rapidjson::Value& forceData = rbData[Serialization::FORCE];
				if (forceData.HasMember(Serialization::VEC2_X) && forceData[Serialization::VEC2_X].IsFloat()) {
					rigidBody.Force.x = forceData[Serialization::VEC2_X].GetFloat();
				}
				if (forceData.HasMember(Serialization::VEC2_Y) && forceData[Serialization::VEC2_Y].IsFloat()) {
					rigidBody.Force.y = forceData[Serialization::VEC2_Y].GetFloat();
				}
			}
			entityMgr.AddComponent<RigidBody>(loadedEntity, rigidBody);
		}

		// Deserialize AABBCollider Component
		if (componentLists.HasMember(Serialization::AABB_COLLIDER_COMPONENT)) {
			const rapidjson::Value& colData = componentLists[Serialization::AABB_COLLIDER_COMPONENT];

			AABBCollider aabbCollider;
			if (colData.HasMember(Serialization::HALF_WIDTH) && colData[Serialization::HALF_WIDTH].IsFloat()) {
				aabbCollider.HalfSize.x = colData[Serialization::HALF_WIDTH].GetFloat();
			}
			if (colData.HasMember(Serialization::HALF_HEIGHT) && colData[Serialization::HALF_HEIGHT].IsFloat()) {
				aabbCollider.HalfSize.y = colData[Serialization::HALF_HEIGHT].GetFloat();
			}
			entityMgr.AddComponent<AABBCollider>(loadedEntity, aabbCollider);
		}

		// Deserialize PlayerState Component
		if (componentLists.HasMember(Serialization::PLAYER_STATE_COMPONENT)) {
			PlayerState playerState;
			entityMgr.AddComponent<PlayerState>(loadedEntity, playerState);
		}

		// Deserialize RotationFlag Component
		if (componentLists.HasMember(Serialization::ROTATIONFLAG_COMPONENT)) {
			const rapidjson::Value& rFlagData = componentLists[Serialization::ROTATIONFLAG_COMPONENT];

			RotationFlag rotationFlag;
			if (rFlagData.HasMember(Serialization::IS_ROTATING) && rFlagData[Serialization::IS_ROTATING].IsBool()) {
				rotationFlag.isRotating = rFlagData[Serialization::IS_ROTATING].GetBool();
			}
			if (rFlagData.HasMember(Serialization::CURRENT_ROTATION) && rFlagData[Serialization::CURRENT_ROTATION].IsFloat()) {
				rotationFlag.currentRotation = rFlagData[Serialization::CURRENT_ROTATION].GetFloat();
			}
			if (rFlagData.HasMember(Serialization::TARGET_ROTATION) && rFlagData[Serialization::TARGET_ROTATION].IsFloat()) {
				rotationFlag.targetRotation = rFlagData[Serialization::TARGET_ROTATION].GetFloat();
			}
			entityMgr.AddComponent<RotationFlag>(loadedEntity, rotationFlag);
		}

		// Deserialize TrapPlatform Component
		if (componentLists.HasMember(Serialization::TRAP_PLATFORM_COMPONENT)) {
			const rapidjson::Value& trapData = componentLists[Serialization::TRAP_PLATFORM_COMPONENT];

			TrapPlatformComponent trapPlatform;
			if (trapData.HasMember(Serialization::IS_TRIGGERED) && trapData[Serialization::IS_TRIGGERED].IsBool()) {
				trapPlatform.isTriggered = trapData[Serialization::IS_TRIGGERED].GetBool();
			}
			if (trapData.HasMember(Serialization::HAS_FALLEN) && trapData[Serialization::HAS_FALLEN].IsBool()) {
				trapPlatform.hasFallen = trapData[Serialization::HAS_FALLEN].GetBool();
			}
			if (trapData.HasMember(Serialization::TRIGGER_DELAY) && trapData[Serialization::TRIGGER_DELAY].IsFloat()) {
				trapPlatform.triggerDelay = trapData[Serialization::TRIGGER_DELAY].GetFloat();
			}
			if (trapData.HasMember(Serialization::CURRENT_DELAY) && trapData[Serialization::CURRENT_DELAY].IsFloat()) {
				trapPlatform.currentDelay = trapData[Serialization::CURRENT_DELAY].GetFloat();
			}
			if (trapData.HasMember(Serialization::FALL_SPEED) && trapData[Serialization::FALL_SPEED].IsFloat()) {
				trapPlatform.fallSpeed = trapData[Serialization::FALL_SPEED].GetFloat();
			}
			if (trapData.HasMember(Serialization::ORIGINAL_POSITION) && trapData[Serialization::ORIGINAL_POSITION].IsObject()) {
				const rapidjson::Value& originalPosData = trapData[Serialization::ORIGINAL_POSITION];
				if (originalPosData.HasMember(Serialization::VEC2_X) && originalPosData[Serialization::VEC2_X].IsFloat()) {
					trapPlatform.originalPosition.x = originalPosData[Serialization::VEC2_X].GetFloat();
				}
				if (originalPosData.HasMember(Serialization::VEC2_Y) && originalPosData[Serialization::VEC2_Y].IsFloat()) {
					trapPlatform.originalPosition.y = originalPosData[Serialization::VEC2_Y].GetFloat();
				}
			}
			if (trapData.HasMember(Serialization::CURRENT_FALL_DISTANCE) && trapData[Serialization::CURRENT_FALL_DISTANCE].IsFloat()) {
				trapPlatform.currentFallDistance = trapData[Serialization::CURRENT_FALL_DISTANCE].GetFloat();
			}
			entityMgr.AddComponent<TrapPlatformComponent>(loadedEntity, trapPlatform);
		}

		// Deserialize ScalingPlatform Component
		if (componentLists.HasMember(Serialization::SCALING_PLATFORM_COMPONENT)) {
			const rapidjson::Value& scalingData = componentLists[Serialization::SCALING_PLATFORM_COMPONENT];

			ScalingPlatformComponent scalingPlatform;
			if (scalingData.HasMember(Serialization::BASE_HALFSIZE) && scalingData[Serialization::BASE_HALFSIZE].IsObject()) {
				const rapidjson::Value& baseHalfSizeData = scalingData[Serialization::BASE_HALFSIZE];
				if (baseHalfSizeData.HasMember(Serialization::VEC2_X) && baseHalfSizeData[Serialization::VEC2_X].IsFloat()) {
					scalingPlatform.baseHalfSize.x = baseHalfSizeData[Serialization::VEC2_X].GetFloat();
				}
				if (baseHalfSizeData.HasMember(Serialization::VEC2_Y) && baseHalfSizeData[Serialization::VEC2_Y].IsFloat()) {
					scalingPlatform.baseHalfSize.y = baseHalfSizeData[Serialization::VEC2_Y].GetFloat();
				}
			}
			if (scalingData.HasMember(Serialization::CURRENT_SCALE) && scalingData[Serialization::CURRENT_SCALE].IsObject()) {
				const rapidjson::Value& currentScaleData = scalingData[Serialization::CURRENT_SCALE];
				if (currentScaleData.HasMember(Serialization::VEC2_X) && currentScaleData[Serialization::VEC2_X].IsFloat()) {
					scalingPlatform.currentScaleX = currentScaleData[Serialization::VEC2_X].GetFloat();
				}
				if (currentScaleData.HasMember(Serialization::VEC2_Y) && currentScaleData[Serialization::VEC2_Y].IsFloat()) {
					scalingPlatform.currentScaleY = currentScaleData[Serialization::VEC2_Y].GetFloat();
				}
			}
			if (scalingData.HasMember(Serialization::SCALE_SPEED) && scalingData[Serialization::SCALE_SPEED].IsFloat()) {
				scalingPlatform.scaleSpeed = scalingData[Serialization::SCALE_SPEED].GetFloat();
			}
			if (scalingData.HasMember(Serialization::MIN_SCALE) && scalingData[Serialization::MIN_SCALE].IsFloat()) {
				scalingPlatform.minScale = scalingData[Serialization::MIN_SCALE].GetFloat();
			}
			if (scalingData.HasMember(Serialization::MAX_SCALE) && scalingData[Serialization::MAX_SCALE].IsFloat()) {
				scalingPlatform.maxScale = scalingData[Serialization::MAX_SCALE].GetFloat();
			}
			if (scalingData.HasMember(Serialization::ELAPSED_TIME) && scalingData[Serialization::ELAPSED_TIME].IsFloat()) {
				scalingPlatform.time = scalingData[Serialization::ELAPSED_TIME].GetFloat();
			}
			entityMgr.AddComponent<ScalingPlatformComponent>(loadedEntity, scalingPlatform);
		}

		// Deserialize TextureComponent Component
		if (componentLists.HasMember(Serialization::TEXTURE_COMPONENT)) {
			const rapidjson::Value& texData = componentLists[Serialization::TEXTURE_COMPONENT];

			TextureComponent textureComp;
			if (texData.HasMember(Serialization::TEXTURE_KEY) && texData[Serialization::TEXTURE_KEY].IsString()) {
				textureComp.textureKey = texData[Serialization::TEXTURE_KEY].GetString();
			}
			if (texData.HasMember(Serialization::HALF_SIZE) && texData[Serialization::HALF_SIZE].IsObject()) {
				const rapidjson::Value& halfSizeData = texData[Serialization::HALF_SIZE];
				if (halfSizeData.HasMember(Serialization::VEC2_X) && halfSizeData[Serialization::VEC2_X].IsFloat()) {
					textureComp.HalfSize.x = halfSizeData[Serialization::VEC2_X].GetFloat();
				}
				if (halfSizeData.HasMember(Serialization::VEC2_Y) && halfSizeData[Serialization::VEC2_Y].IsFloat()) {
					textureComp.HalfSize.y = halfSizeData[Serialization::VEC2_Y].GetFloat();
				}
			}
			entityMgr.AddComponent<TextureComponent>(loadedEntity, textureComp);
		}

		// Deserialize AudioComponent Component
		if (componentLists.HasMember(Serialization::AUDIO_COMPONENT)) {
			const rapidjson::Value& audioData = componentLists[Serialization::AUDIO_COMPONENT];

			AudioComponent audioComp;
			if (audioData.HasMember(Serialization::SOUND_KEY) && audioData[Serialization::SOUND_KEY].IsString()) {
				audioComp.soundKey = audioData[Serialization::SOUND_KEY].GetString();
			}
			if (audioData.HasMember(Serialization::SHOULD_LOOP) && audioData[Serialization::SHOULD_LOOP].IsBool()) {
				audioComp.loop = audioData[Serialization::SHOULD_LOOP].GetBool();
			}
			if (audioData.HasMember(Serialization::IS_STREAMING) && audioData[Serialization::IS_STREAMING].IsBool()) {
				audioComp.stream = audioData[Serialization::IS_STREAMING].GetBool();
			}
			if (audioData.HasMember(Serialization::VOLUME) && audioData[Serialization::VOLUME].IsFloat()) {
				audioComp.volume = audioData[Serialization::VOLUME].GetFloat();
			}
			entityMgr.AddComponent<AudioComponent>(loadedEntity, audioComp);
		}

		// Deserialize AnimationComponent Component
		if (componentLists.HasMember(Serialization::ANIMATION_COMPONENT)) {
			const rapidjson::Value& animData = componentLists[Serialization::ANIMATION_COMPONENT];

			AnimationComponent animComp;
			if (animData.HasMember(Serialization::CURRENT_STATE) && animData[Serialization::CURRENT_STATE].IsInt()) {
				animComp.currentState = static_cast<AnimationState>(animData[Serialization::CURRENT_STATE].GetInt());
			}
			if (animData.HasMember(Serialization::CURRENT_FRAME) && animData[Serialization::CURRENT_FRAME].IsInt()) {
				animComp.currentFrame = animData[Serialization::CURRENT_FRAME].GetInt();
			}
			if (animData.HasMember(Serialization::FRAME_DURATION) && animData[Serialization::FRAME_DURATION].IsFloat()) {
				animComp.frameDuration = animData[Serialization::FRAME_DURATION].GetFloat();
			}
			if (animData.HasMember(Serialization::START_FRAME) && animData[Serialization::START_FRAME].IsInt()) {
				animComp.startFrame = animData[Serialization::START_FRAME].GetInt();
			}
			if (animData.HasMember(Serialization::FRAME_COUNT) && animData[Serialization::FRAME_COUNT].IsInt()) {
				animComp.frameCount = animData[Serialization::FRAME_COUNT].GetInt();
			}
			if (animData.HasMember(Serialization::ELAPSED_TIME) && animData[Serialization::ELAPSED_TIME].IsFloat()) {
				animComp.elapsedTime = animData[Serialization::ELAPSED_TIME].GetFloat();
			}
			if (animData.HasMember(Serialization::IS_PLAYING) && animData[Serialization::IS_PLAYING].IsBool()) {
				animComp.isPlaying = animData[Serialization::IS_PLAYING].GetBool();
			}
			if (animData.HasMember(Serialization::IS_LOOPING) && animData[Serialization::IS_LOOPING].IsBool()) {
				animComp.isLooping = animData[Serialization::IS_LOOPING].GetBool();
			}
			entityMgr.AddComponent<AnimationComponent>(loadedEntity, animComp);
		}

		// Deserialize TextComponent Component
		if (componentLists.HasMember(Serialization::TEXT_COMPONENT)) {
			const rapidjson::Value& textData = componentLists[Serialization::TEXT_COMPONENT];
			TextComponent textComp;
			if (textData.HasMember(Serialization::TEXT) && textData[Serialization::TEXT].IsString()) {
				textComp.textToDraw = textData[Serialization::TEXT].GetString();
			}
			if (textData.HasMember(Serialization::FONT_KEY) && textData[Serialization::FONT_KEY].IsString()) {
				textComp.fontKey = textData[Serialization::FONT_KEY].GetString();
			}
			if (textData.HasMember(Serialization::FONT_COLOR) && textData[Serialization::FONT_COLOR].IsArray()) {
				const rapidjson::Value& colorArr = textData[Serialization::FONT_COLOR];
				textComp.color.r = colorArr[0].GetFloat();
				textComp.color.g = colorArr[1].GetFloat();
				textComp.color.b = colorArr[2].GetFloat();
				textComp.color.a = colorArr[3].GetFloat();
			}
			if (textData.HasMember(Serialization::FONT_SIZE) && textData[Serialization::FONT_SIZE].IsFloat()) {
				textComp.scale = textData[Serialization::FONT_SIZE].GetFloat();
			}
			entityMgr.AddComponent<TextComponent>(loadedEntity, textComp);
		}

		// Deserialize Camera Component
		if (componentLists.HasMember(Serialization::CAMERA_COMPONENT)) {
			const rapidjson::Value& cameraData = componentLists[Serialization::CAMERA_COMPONENT];
			Camera cameraComp;
			if (cameraData.HasMember(Serialization::POSITION) && cameraData[Serialization::POSITION].IsObject()) {
				const rapidjson::Value& positionData = cameraData[Serialization::POSITION];
				if (positionData.HasMember(Serialization::VEC2_X) && positionData[Serialization::VEC2_X].IsFloat()) {
					cameraComp.position.x = positionData[Serialization::VEC2_X].GetFloat();
				}
				if (positionData.HasMember(Serialization::VEC2_Y) && positionData[Serialization::VEC2_Y].IsFloat()) {
					cameraComp.position.y = positionData[Serialization::VEC2_Y].GetFloat();
				}
			}
			if (cameraData.HasMember(Serialization::SIZE) && cameraData[Serialization::SIZE].IsObject()) {
				const rapidjson::Value& sizeData = cameraData[Serialization::SIZE];
				if (sizeData.HasMember(Serialization::WIDTH) && sizeData[Serialization::WIDTH].IsFloat()) {
					cameraComp.size.x = sizeData[Serialization::WIDTH].GetFloat();
				}
				if (sizeData.HasMember(Serialization::HEIGHT) && sizeData[Serialization::HEIGHT].IsFloat()) {
					cameraComp.size.y = sizeData[Serialization::HEIGHT].GetFloat();
				}
			}
			if (cameraData.HasMember(Serialization::ZOOM) && cameraData[Serialization::ZOOM].IsFloat()) {
				cameraComp.zoom = cameraData[Serialization::ZOOM].GetFloat();
			}
			if (cameraData.HasMember(Serialization::TARGET_ENTITY) && cameraData[Serialization::TARGET_ENTITY].IsUint64()) {
				cameraComp.targetEntity = (uint32_t)cameraData[Serialization::TARGET_ENTITY].GetUint64();
			}
			if (cameraData.HasMember(Serialization::FOLLOW_SPEED) && cameraData[Serialization::FOLLOW_SPEED].IsFloat()) {
				cameraComp.followSpeed = cameraData[Serialization::FOLLOW_SPEED].GetFloat();
			}
			if (cameraData.HasMember(Serialization::IS_ACTIVE) && cameraData[Serialization::IS_ACTIVE].IsBool()) {
				cameraComp.isActive = cameraData[Serialization::IS_ACTIVE].GetBool();
			}
			entityMgr.AddComponent<Camera>(loadedEntity, cameraComp);
		}

		// Deserialize SceneOwned Component
		if (componentLists.HasMember(Serialization::SCENE_OWNED_COMPONENT)) {
			const rapidjson::Value& sceneOwnedData = componentLists[Serialization::SCENE_OWNED_COMPONENT];
			SceneOwned sceneOwnedComp;
			if (sceneOwnedData.HasMember(Serialization::COMP_SCENE_NAME) && sceneOwnedData[Serialization::COMP_SCENE_NAME].IsString()) {
				sceneOwnedComp.sceneName = sceneOwnedData[Serialization::COMP_SCENE_NAME].GetString();
			}
			
			entityMgr.AddComponent<SceneOwned>(loadedEntity, sceneOwnedComp);
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
			}
		}
	}

	Entity EntitySerializer::DeserializeEntity(EntityStorage& ecs, const rapidjson::Value& entityData) {
		EntityManager entityMgr(ecs);

		Entity loadedEntity = entityMgr.CreateEntity();

		/* Entity Properties Deserialization */
		EntityProperties& properties = entityMgr.GetProperties(loadedEntity);
		if (entityData.HasMember(Serialization::ENTITY_NAME) && entityData[Serialization::ENTITY_NAME].IsString()) {
			properties.name = entityData[Serialization::ENTITY_NAME].GetString();
		}
		if (entityData.HasMember(Serialization::ENTITY_LAYER) && entityData[Serialization::ENTITY_LAYER].IsInt()) {
			properties.layer = static_cast<CollisionLayer>(entityData[Serialization::ENTITY_LAYER].GetInt());
		}

		/* Components Deserialization */
		if (entityData.HasMember(Serialization::COMPONENTS)) {
			if (entityData[Serialization::COMPONENTS].IsObject()) {
				DeserializeComponents(entityMgr, loadedEntity, entityData);
			}
			else {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Components data is not an object." << std::endl;
			}
		}

		/* Tags Deserialization */
		if (entityData.HasMember(Serialization::TAGS)) {
			if (entityData[Serialization::TAGS].IsArray()) {
				DeserializeTags(entityMgr, loadedEntity, entityData);
			}
			else {
				Logger::Get().Error(DebugType::EntitySeralizer) << "Tags data is not a list." << std::endl;
			}
		}

		return loadedEntity;
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
	}
}