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
		rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
		EntityManager entityMgr(ecs);

		// Entity Properties Serialization
		EntityProperties& properties = entityMgr.GetProperties(entity);
		doc.AddMember("name", rapidjson::StringRef(properties.name.c_str()), alloc);
		doc.AddMember("layer", rapidjson::StringRef(LayerToString(properties.layer)), alloc);

		rapidjson::Value componentList(rapidjson::kObjectType);
		
		// Serialize PlayerStats Component
		if (entityMgr.HasComponent(entity, ComponentType::PlayerStatsType)) {
			PlayerStats& stats = entityMgr.GetComponent<PlayerStats>(entity);
			rapidjson::Value value(rapidjson::kObjectType);
			value.AddMember("moveSpeed", stats.moveSpeed, alloc);
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

		doc.AddMember("components", componentList, alloc);
	}

	void EntitySerializer::SerializeEntityToFile(Entity entity, EntityStorage& ecs, const std::string& filepath) {
		rapidjson::Document doc;
		doc.SetObject();

		Logger::Get().Info(DebugType::EntitySeralizer) << "Serializing entity to " << filepath << std::endl;
		SerializeEntityInternal(entity, ecs, doc);
		Logger::Get().Info(DebugType::EntitySeralizer) << "writing data to Data/" << filepath << std::endl;
		FileIOSystem::WriteFileJson("Data/" + filepath, doc);
	}
}