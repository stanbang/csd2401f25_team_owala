#include "EntitySerializer.h"
#include "EntityManager.h"
#include "FileUtility.h"
#include "Logger.h"

// Internal Helpers - Will move this and the Editor's version to a global utils file later.
namespace {
	using namespace Framework;

	std::string ComponentToString(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PlayerStatsType:				return "PlayerStats";
		case ComponentType::PositionType:					return "Position";
		case ComponentType::VelocityType:					return "Velocity";
		case ComponentType::AccelerationType:				return "Acceleration";
		case ComponentType::TriangleType:					return "Triangle";
		case ComponentType::RigidBodyType:					return "RigidBody";
		case ComponentType::AABBColliderType:				return "AABBCollider";
		case ComponentType::PlayerStateType:				return "PlayerState";
		case ComponentType::RotationFlagType:				return "RotationFlag";
		case ComponentType::TrapPlatformComponentType:		return "TrapPlatform";
		case ComponentType::ScalingPlatformComponentType:	return "ScalingPlatform";
		case ComponentType::TextureComponentType:			return "Texture";
		case ComponentType::AudioComponentType:				return "Audio";
		case ComponentType::AnimationComponentType:			return "Animation";
		case ComponentType::TextComponentType:				return "Text";
		case ComponentType::PlayerTagType:					return "PlayerTag";
		case ComponentType::PlatformTagType:				return "PlatformTag";
		case ComponentType::BackgroundTagType:				return "BackgroundTag";
		case ComponentType::EnemyTagType:					return "EnemyTag";
		case ComponentType::SceneOwnedType:					return "SceneOwned";
		case ComponentType::PersistentTagType:				return "PersistentTag";
		case ComponentType::ActiveTagType:					return "ActiveTag";
		default:											return "Unknown";
		}
	}

	// Helper to convert a CollisionLayer enum to its string representation.
	const char* LayerToString(CollisionLayer layer) {
		switch (layer) {
		case CollisionLayer::Default:   return "Default";
		case CollisionLayer::Player:    return "Player";
		case CollisionLayer::Platform:  return "Platform";
		case CollisionLayer::Enemy:     return "Enemy";
		case CollisionLayer::Trigger:   return "Trigger";
		default:                        return "Invalid";
		}
	}

	static void WriteVector2D(rapidjson::Value& parent, const char* name, const Vector2D& vec, rapidjson::Document::AllocatorType& alloc) {
		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("x", vec.x, alloc);
		obj.AddMember("y", vec.y, alloc);
		parent.AddMember(rapidjson::StringRef(name), obj, alloc);
	}

	static Vector2D ReadVector2D(const rapidjson::Value& obj, const char* name) {
		const rapidjson::Value& v = obj[name];
		Vector2D vec;
		vec.x = v["x"].GetFloat();
		vec.y = v["y"].GetFloat();
		return vec;
	}
}

namespace Framework {
	void EntitySerializer::SerializeEntityInternal(Entity entity, EntityStorage& ecs, rapidjson::Document& doc) {
		doc.SetObject();
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
		EntityManager entityMgr(ecs);

		/* Entity Properties Serialization */
		EntityProperties& properties = entityMgr.GetProperties(entity);
		doc.AddMember("name", rapidjson::StringRef(properties.name.c_str()), alloc);
		doc.AddMember("layer", rapidjson::StringRef(LayerToString(properties.layer)), alloc);

		rapidjson::Value componentList(rapidjson::kObjectType);
		
		/* Components Serialization */
		// Serialize PlayerStats Component
		if (entityMgr.HasComponent(entity, ComponentType::PlayerStatsType)) {
			PlayerStats& stats = entityMgr.GetComponent<PlayerStats>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("move_speed", stats.moveSpeed, alloc);
			componentList.AddMember("PlayerStats", value, alloc);
		}

		// Serialize Position Component
		if (entityMgr.HasComponent(entity, ComponentType::PositionType)) {
			Position& pos = entityMgr.GetComponent<Position>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("x", pos.Pos.x, alloc);
			value.AddMember("y", pos.Pos.y, alloc);
			componentList.AddMember("Position", value, alloc);
		}

		// Serialize Velocity Component
		if (entityMgr.HasComponent(entity, ComponentType::VelocityType)) {
			Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("x", vel.Vel.x, alloc);
			value.AddMember("y", vel.Vel.y, alloc);
			componentList.AddMember("Velocity", value, alloc);
		}

		// Serialize Acceleration Component
		if (entityMgr.HasComponent(entity, ComponentType::AccelerationType)) {
			Acceleration& acc = entityMgr.GetComponent<Acceleration>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("x", acc.Accel.x, alloc);
			value.AddMember("y", acc.Accel.y, alloc);
			componentList.AddMember("Acceleration", value, alloc);
		}

		// Serialize Triangle Component
		if (entityMgr.HasComponent(entity, ComponentType::TriangleType)) {
			Triangle& triangle = entityMgr.GetComponent<Triangle>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("vertex_a", triangle.a, alloc);
			value.AddMember("vertex_b", triangle.b, alloc);
			value.AddMember("vertex_c", triangle.c, alloc);
			componentList.AddMember("Triangle", value, alloc);
		}

		// Serialize RigidBody Component
		if (entityMgr.HasComponent(entity, ComponentType::RigidBodyType)) {
			RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("mass", rb.Mass, alloc);
			value.AddMember("inv_mass", rb.InverseMass, alloc);
			rapidjson::Value force(rapidjson::kObjectType);
			force.AddMember("x", rb.Force.x, alloc);
			force.AddMember("y", rb.Force.y, alloc);
			value.AddMember("force", force, alloc);
			componentList.AddMember("RigidBody", value, alloc);
		}

		// Serialize AABBCollider Component
		if (entityMgr.HasComponent(entity, ComponentType::AABBColliderType)) {
			AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("half_width", col.HalfSize.x, alloc);
			value.AddMember("half_height", col.HalfSize.y, alloc);
			componentList.AddMember("AABBCollider", value, alloc);
		}

		// Serialize PlayerState Component
		if (entityMgr.HasComponent(entity, ComponentType::PlayerStateType)) {
			PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			componentList.AddMember("PlayerState", value, alloc);
		}

		// Serialize RotationFlag Component
		if (entityMgr.HasComponent(entity, ComponentType::RotationFlagType)) {
			RotationFlag& rFlag = entityMgr.GetComponent<RotationFlag>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("is_rotating", rFlag.isRotating, alloc);
			value.AddMember("current_rotation", rFlag.currentRotation, alloc);
			value.AddMember("target_rotation", rFlag.targetRotation, alloc);
			componentList.AddMember("RotationFlag", value, alloc);
		}

		// Serialize TrapPlatform Component
		if (entityMgr.HasComponent(entity, ComponentType::TrapPlatformComponentType)) {
			TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("is_triggered", trap.isTriggered, alloc);
			value.AddMember("has_fallen", trap.hasFallen, alloc);
			value.AddMember("trigger_delay", trap.triggerDelay, alloc);
			value.AddMember("current_delay", trap.currentDelay, alloc);
			value.AddMember("fall_speed", trap.fallSpeed, alloc);
			rapidjson::Value originalPos(rapidjson::kObjectType);
			originalPos.AddMember("x", trap.originalPosition.x, alloc);
			originalPos.AddMember("y", trap.originalPosition.y, alloc);
			value.AddMember("original_position", originalPos, alloc);
			value.AddMember("current_fall_distance", trap.currentFallDistance, alloc);
			componentList.AddMember("TrapPlatform", value, alloc);
		}

		// Serialize ScalingPlatform Component
		if (entityMgr.HasComponent(entity, ComponentType::ScalingPlatformComponentType)) {
			ScalingPlatformComponent& scalingPlatform = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			rapidjson::Value baseHalfSize(rapidjson::kObjectType);
			baseHalfSize.AddMember("x", scalingPlatform.baseHalfSize.x, alloc);
			baseHalfSize.AddMember("y", scalingPlatform.baseHalfSize.y, alloc);
			value.AddMember("base_half_size", baseHalfSize, alloc);
			rapidjson::Value currentScale (rapidjson::kObjectType);
			currentScale.AddMember("x", scalingPlatform.currentScaleX, alloc);
			currentScale.AddMember("y", scalingPlatform.currentScaleY, alloc);
			value.AddMember("current_scale", currentScale, alloc);
			value.AddMember("scale_speed", scalingPlatform.scaleSpeed, alloc);
			value.AddMember("min_scale", scalingPlatform.minScale, alloc);
			value.AddMember("max_scale", scalingPlatform.maxScale, alloc);
			value.AddMember("time_elapsed", scalingPlatform.time, alloc);
			rapidjson::Value transformMtx(rapidjson::kArrayType);
			for (int i = 0; i < 3; ++i) {
				for (int j = 0; j < 3; ++j) {
					transformMtx.PushBack(transformMtx[i][j], alloc);
				}
			}
			value.AddMember("transform_matrix", transformMtx, alloc);
			componentList.AddMember("ScalingPlatform", value, alloc);
		}

		// Serialize TextureComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextureComponentType)) {
			TextureComponent& texComp = entityMgr.GetComponent<TextureComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("texture_key", rapidjson::StringRef(texComp.textureKey.c_str()), alloc);
			rapidjson::Value halfSize(rapidjson::kObjectType);
			halfSize.AddMember("x", texComp.HalfSize.x, alloc);
			halfSize.AddMember("y", texComp.HalfSize.y, alloc);
			value.AddMember("half_size", halfSize, alloc);
			componentList.AddMember("TextureComponent", value, alloc);
		}

		// Serialize AudioComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AudioComponentType)) {
			AudioComponent& audioComp = entityMgr.GetComponent<AudioComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("sound_key", rapidjson::StringRef(audioComp.soundKey.c_str()), alloc);
			value.AddMember("should_loop", audioComp.loop, alloc);
			value.AddMember("is_streaming", audioComp.stream, alloc);
			value.AddMember("volume", audioComp.volume, alloc);
			componentList.AddMember("AudioComponent", value, alloc);
		}

		// Serialize AnimationComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::AnimationComponentType)) {
			AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("current_state", static_cast<int>(animComp.currentState), alloc);
			value.AddMember("current_frame", animComp.currentFrame, alloc);
			value.AddMember("frame_duration", animComp.frameDuration, alloc);
			value.AddMember("start_frame", animComp.startFrame, alloc);
			value.AddMember("frame_count", animComp.frameCount, alloc);
			value.AddMember("elapsed_time", animComp.elapsedTime, alloc);
			value.AddMember("is_playing", animComp.isPlaying, alloc);
			value.AddMember("is_looping", animComp.isLooping, alloc);
			componentList.AddMember("AnimationComponent", value, alloc);
		}

		// Serialize TextComponent Component
		if (entityMgr.HasComponent(entity, ComponentType::TextComponentType)) {
			TextComponent& textComp = entityMgr.GetComponent<TextComponent>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("text", rapidjson::StringRef(textComp.textToDraw.c_str()), alloc);
			value.AddMember("font_key", rapidjson::StringRef(textComp.fontName.c_str()), alloc);
			rapidjson::Value colorList(rapidjson::kArrayType);
			colorList.PushBack(textComp.color.r, alloc);
			colorList.PushBack(textComp.color.g, alloc);
			colorList.PushBack(textComp.color.b, alloc);
			colorList.PushBack(textComp.color.a, alloc);
			value.AddMember("font_color", colorList, alloc);
			value.AddMember("font_size", textComp.scale, alloc);
			componentList.AddMember("TextComponent", value, alloc);
		}

		// Serialize Camera Component
		if (entityMgr.HasComponent(entity, ComponentType::CameraType)) {
			Camera& cameraComp = entityMgr.GetComponent<Camera>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			rapidjson::Value positionObj(rapidjson::kObjectType);
			positionObj.AddMember("x", cameraComp.position.x, alloc);
			positionObj.AddMember("y", cameraComp.position.y, alloc);
			value.AddMember("position", positionObj, alloc);
			rapidjson::Value sizeObj(rapidjson::kObjectType);
			sizeObj.AddMember("width", cameraComp.size.x, alloc);
			sizeObj.AddMember("height", cameraComp.size.y, alloc);
			value.AddMember("size", sizeObj, alloc);
			value.AddMember("zoom", cameraComp.zoom, alloc);
			value.AddMember("target_entity", cameraComp.targetEntity, alloc);
			value.AddMember("follow_speed", cameraComp.followSpeed, alloc);
			value.AddMember("is_active", cameraComp.isActive, alloc);
			componentList.AddMember("Camera", value, alloc);
		}

		doc.AddMember("components", componentList, alloc);

		/* Tags Serialization */
		// Serialize Tags into an array
		rapidjson::Value tagsList(rapidjson::kArrayType);

		if (entityMgr.HasComponent(entity, ComponentType::PlayerTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::PlayerTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::PlatformTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::PlatformTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::BackgroundTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::BackgroundTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::EnemyTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::EnemyTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::SceneOwnedType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::SceneOwnedType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::PersistentTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::PersistentTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}
		if (entityMgr.HasComponent(entity, ComponentType::ActiveTagType)) {
			rapidjson::Value tagVal(ComponentToString(ComponentType::ActiveTagType).c_str(), alloc);
			tagsList.PushBack(tagVal, alloc);
		}

		doc.AddMember("tags", tagsList, alloc);
	}

	void EntitySerializer::SerializeEntityToFile(Entity entity, EntityStorage& ecs, const std::string& filepath) {
		rapidjson::Document doc;

		Logger::Get().Info(DebugType::EntitySeralizer) << "Serializing entity to " << filepath << std::endl;
		SerializeEntityInternal(entity, ecs, doc);
		Logger::Get().Info(DebugType::EntitySeralizer) << "writing data to Data/" << filepath << std::endl;
		FileIOSystem::WriteFileJson("Data/" + filepath, doc);
	}
}