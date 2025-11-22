/*****************************************************************//**
 * \file	EntityManager.h
 * \brief	Manager responsible for managing entities and their
 *			components in the ECS architecture. Also serves as a
 *			Entity Factory to create entities.
 *
 *
 * \author	Jason Lou
 * \date	26 September 2025
 *********************************************************************/
#include "EntityManager.h"
#include "Logger.h"
#include "ScriptRegistry.h"

namespace Framework {
#pragma region CURRENT ENTITYMANAGER

	// ---------------------------------------------------------------------------
	// Constructors
	// ---------------------------------------------------------------------------
	EntityManager::EntityManager(EntityStorage& entityStorage) : ecs(entityStorage) {};

	// ---------------------------------------------------------------------------
	// EntityManager APIs
	// ---------------------------------------------------------------------------
	Entity EntityManager::CreateEntity() {
		ComponentSignature emptySignature; // New entity starts with no components
		Entity newEntity = nextEntityID++;
		ecs.entitySignatures[newEntity] = emptySignature;

		std::string entityName = "GameObject (" + std::to_string(std::clamp(static_cast<unsigned>(newEntity), 0U, UINT_MAX)) + ")";
		ecs.properties[newEntity] = EntityProperties{ entityName, CollisionLayer::Default };

		return newEntity;
	}

	Entity EntityManager::CreateEntityWithID(Entity entityID) {
		ComponentSignature emptySignature; // New entity starts with no components
		ecs.entitySignatures[entityID] = emptySignature;

		std::string entityName = "GameObject (" + std::to_string(std::clamp(static_cast<unsigned>(entityID), 0U, UINT_MAX)) + ")";
		ecs.properties[entityID] = EntityProperties{ entityName, CollisionLayer::Default };

		if (entityID >= nextEntityID) {
			nextEntityID = entityID + 1;
		}

		return entityID;
	}

	Entity EntityManager::CloneEntity(const Entity& entity) {
		if (ecs.entitySignatures.find(entity) == ecs.entitySignatures.end()) {
			throw std::runtime_error("Attempted to clone a non existent entity!");
		}

		Entity newEntity = CreateEntity();
		GetProperties(newEntity) = GetProperties(entity);
		ComponentSignature otherSignature = ecs.entitySignatures[entity];

		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);

			if (HasComponent(entity, componentType)) {
				switch (componentType) {
					case ComponentType::PositionType: AddComponent<Position>(newEntity, GetComponent<Position>(entity)); break;
					case ComponentType::VelocityType: AddComponent<Velocity>(newEntity, GetComponent<Velocity>(entity)); break;
					case ComponentType::AccelerationType: AddComponent<Acceleration>(newEntity, GetComponent<Acceleration>(entity)); break;
					case ComponentType::TriangleType: AddComponent<Triangle>(newEntity, GetComponent<Triangle>(entity)); break;
					case ComponentType::RigidBodyType: AddComponent<RigidBody>(newEntity, GetComponent<RigidBody>(entity)); break;
					case ComponentType::AABBColliderType: AddComponent<AABBCollider>(newEntity, GetComponent<AABBCollider>(entity)); break;
					case ComponentType::PlayerStateType: AddComponent<PlayerState>(newEntity, GetComponent<PlayerState>(entity)); break;
					case ComponentType::RotationFlagType: AddComponent<RotationFlag>(newEntity, GetComponent<RotationFlag>(entity)); break;
					case ComponentType::TrapPlatformComponentType: AddComponent<TrapPlatformComponent>(newEntity, GetComponent<TrapPlatformComponent>(entity)); break;
					case ComponentType::ScalingPlatformComponentType: AddComponent<ScalingPlatformComponent>(newEntity, GetComponent<ScalingPlatformComponent>(entity)); break;
					case ComponentType::ButtonType: AddComponent<Button>(newEntity, GetComponent<Button>(entity)); break;
					case ComponentType::TextureComponentType: AddComponent<TextureComponent>(newEntity, GetComponent<TextureComponent>(entity)); break;
					case ComponentType::AudioComponentType: AddComponent<AudioComponent>(newEntity, GetComponent<AudioComponent>(entity)); break;
					case ComponentType::AnimationComponentType: AddComponent<AnimationComponent>(newEntity, GetComponent<AnimationComponent>(entity)); break;
					case ComponentType::TextComponentType: 	AddComponent<TextComponent>(newEntity, GetComponent<TextComponent>(entity)); break;
					case ComponentType::SceneOwnedType: AddComponent<SceneOwned>(newEntity, GetComponent<SceneOwned>(entity)); break;
					case ComponentType::BirdAIComponentType: AddComponent<BirdAIComponent>(newEntity, GetComponent<BirdAIComponent>(entity)); break;
					case ComponentType::SkeletonAIComponentType: AddComponent<SkeletonAIComponent>(newEntity, GetComponent<SkeletonAIComponent>(entity)); break; 
					case ComponentType::RotationType: AddComponent<Rotation>(newEntity, GetComponent<Rotation>(entity)); break;
					case ComponentType::ScaleType: AddComponent<Scale>(newEntity, GetComponent<Scale>(entity)); break;
					default:
						break;
				}
			}
		}

		ecs.entitySignatures[newEntity] = otherSignature;

		return newEntity;
	}

	void EntityManager::DestroyEntity(Entity entity) {
		// True Destruction of an entity
		ComponentSignature signature = ecs.entitySignatures[entity];
		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);
			if (signature.test(componentType)) {
				RemoveComponent(entity, componentType);
			}
		}

		ecs.entitySignatures.erase(entity);
		ecs.properties.erase(entity);
	}

	EntityProperties& EntityManager::GetProperties(Entity entity) { 
		if (ecs.properties.find(entity) != ecs.properties.end()) {
			return ecs.properties.at(entity);
		}
		else {
			throw GameException(DebugType::Editor, "Corrupted Object! Properties are invalid. Please use EntityManager's CreateEntity API to instantiate objects!");
		}
	}

	bool EntityManager::ValidateEntity(Entity entity) {
		if (ecs.properties.find(entity) == ecs.properties.end()) {
			return false;
		}
		
		return true;
	}

	bool EntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		return ecs.entitySignatures[entity].test(componentType);
	}

	void EntityManager::AddComponent(Entity entity, ComponentType componentType) {
		if (HasComponent(entity, componentType)) return;

		switch (componentType)
		{
		case ComponentType::PositionType: AddComponent<Position>(entity, Position{}); break;
		case ComponentType::SpawnPointType: AddComponent<SpawnPoint>(entity, SpawnPoint{}); break;
		case ComponentType::VelocityType: AddComponent<Velocity>(entity, Velocity{}); break;
		case ComponentType::AccelerationType: AddComponent<Acceleration>(entity, Acceleration{}); break;
		case ComponentType::TriangleType: AddComponent<Triangle>(entity, Triangle{}); break;
		case ComponentType::RigidBodyType: AddComponent<RigidBody>(entity, RigidBody{}); break;
		case ComponentType::AABBColliderType: AddComponent<AABBCollider>(entity, AABBCollider{}); break;
		case ComponentType::PlayerStateType: AddComponent<PlayerState>(entity, PlayerState{}); break;
		case ComponentType::RotationFlagType: AddComponent<RotationFlag>(entity, RotationFlag{}); break;
		case ComponentType::TrapPlatformComponentType: AddComponent<TrapPlatformComponent>(entity, TrapPlatformComponent{}); break;
		case ComponentType::ScalingPlatformComponentType: AddComponent<ScalingPlatformComponent>(entity, ScalingPlatformComponent{}); break;
		case ComponentType::TextureComponentType: AddComponent<TextureComponent>(entity, TextureComponent{}); break;
		case ComponentType::AudioComponentType: AddComponent<AudioComponent>(entity, AudioComponent{}); break;
		case ComponentType::AnimationComponentType: AddComponent<AnimationComponent>(entity, AnimationComponent{}); break;
		case ComponentType::TextComponentType: 	AddComponent<TextComponent>(entity, TextComponent{}); break;
		case ComponentType::ButtonType: AddComponent<Button>(entity, Button{}); break;
		case ComponentType::CameraType: AddComponent<Camera>(entity, Camera{}); break;
		case ComponentType::NativeScriptsComponentType: AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{}); break;
		case ComponentType::BirdAIComponentType: AddComponent<BirdAIComponent>(entity, BirdAIComponent{}); break;
		case ComponentType::SkeletonAIComponentType: AddComponent<SkeletonAIComponent>(entity, SkeletonAIComponent{}); break; 
		case ComponentType::PlayerTagType:
		case ComponentType::PlatformTagType:
		case ComponentType::BackgroundTagType:
		case ComponentType::EnemyTagType: ; break;
		case ComponentType::SceneOwnedType: AddComponent<SceneOwned>(entity, SceneOwned{}); break;
		case ComponentType::PersistentTagType:
		case ComponentType::ActiveTagType: break;
		case ComponentType::GUITagType: break;
		case ComponentType::RotationType: AddComponent<Rotation>(entity, Rotation{}); break;
		case ComponentType::ScaleType: AddComponent<Scale>(entity, Scale{}); break;

		default:
			Logger::Get().Error(DebugType::GameManager) << "AddComponent: Unsupported ComponentType " << static_cast<int>(componentType);
			return;
		}

		ecs.entitySignatures[entity].set(componentType);
	}

	void EntityManager::RemoveComponent(Entity entity, ComponentType componentType) {
		if (!HasComponent(entity, componentType)) return;

		switch (componentType) {
			case PositionType:      ecs.position.erase(entity); break;
			case SpawnPointType:    ecs.spawnPoint.erase(entity); break;
			case VelocityType:      ecs.velocity.erase(entity); break;
			case AccelerationType:  ecs.acceleration.erase(entity); break;
			case TriangleType:      ecs.triangle.erase(entity); break;
			case RigidBodyType:     ecs.rigidBody.erase(entity); break;
			case AABBColliderType:  ecs.colliders.erase(entity); break;
			case PlayerStateType:   ecs.playerState.erase(entity); break;
			case RotationFlagType:  ecs.rotationFlag.erase(entity); break;
			case TrapPlatformComponentType: ecs.trapPlatform.erase(entity); break;
			case ScalingPlatformComponentType: ecs.scalingPlatform.erase(entity); break;
			case TextureComponentType: ecs.tex.erase(entity); break;
			case AudioComponentType: ecs.audio.erase(entity); break;
			case AnimationComponentType: ecs.animatedTex.erase(entity); break;
			case TextComponentType: ecs.text.erase(entity); break;
			case CameraType: ecs.cameras.erase(entity); break;
			case NativeScriptsComponentType: {
				std::unordered_map<Entity, NativeScriptsComponent>::iterator it = ecs.nativeScripts.find(entity);
				if (it != ecs.nativeScripts.end()) {
					NativeScriptsComponent& nativeScripts = it->second;
					for (NativeScriptsComponent::NativeScript& script : nativeScripts.scripts) {
						script.Reset();
					}

					ecs.nativeScripts.erase(entity);
				}
				break;
			}
			case ButtonType: ecs.button.erase(entity); break;
			case BirdAIComponentType: ecs.birdAIComponent.erase(entity); break;
			case SkeletonAIComponentType: ecs.skeletonAIComponent.erase(entity); break; 
			case PlayerTagType:
			case PlatformTagType:
			case BackgroundTagType:
			case EnemyTagType:; break;
			case SceneOwnedType: ecs.sceneOwned.erase(entity); break;
			case PersistentTagType:
			case ActiveTagType: break;
			case GUITagType: break;
			case RotationType: ecs.rotation.erase(entity); break;
			case ScaleType: ecs.scale.erase(entity); break;
			default: 
				Logger::Get().Error(DebugType::GameManager) << "AddComponent: Unsupported ComponentType " << static_cast<int>(componentType);
				return;
		}

		ecs.entitySignatures[entity].reset(componentType);
	}

	bool EntityManager::IsTagComponent(ComponentType componentType) {
		switch (componentType)
		{
		case ComponentType::PlayerTagType:
		case ComponentType::PlatformTagType:
		case ComponentType::BackgroundTagType:
		case ComponentType::EnemyTagType:
		case ComponentType::PersistentTagType:
		case ComponentType::ActiveTagType:
		case ComponentType::GUITagType:
			return true;
		default:
			return false;
		}
	}

	void EntityManager::ClearTags(Entity entity) {
		for (int i = 0; i < ComponentType::MAX_COMPONENTS; ++i) {
			ComponentType componentType = static_cast<ComponentType>(i);
			if (IsTagComponent(componentType)) {
				RemoveComponent(entity, componentType);
			}
		}
	}

	std::size_t EntityManager::AttachScriptByName(Entity entity, std::string typeName, bool enabled) {
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{});
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);
		nativeScripts.scripts.emplace_back();
		NativeScriptsComponent::NativeScript& scriptToAdd = nativeScripts.scripts.back();
		scriptToAdd.enabled = enabled;

		if (!ScriptRegistry::Get().BindScriptByName(scriptToAdd, typeName)) {
			nativeScripts.scripts.pop_back();
			Logger::Get().Error(DebugType::GameManager)	<< "AttachScriptByName: Unknown Script '" << typeName << "'" << std::endl;
			return static_cast<std::size_t>(-1);
		}

		return nativeScripts.scripts.size() - 1;
	}

	void EntityManager::RemoveScriptAt(Entity entity, std::size_t index, bool removeComponentIfEmpty) {
		if (!HasComponent(entity, NativeScriptsComponentType)) return;

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		if (index >= nativeScripts.scripts.size()) {
			Logger::Get().Info(DebugType::GameManager) << "RemoveScriptAt: Provided index is greater than the number of scripts this entity has." << std::endl;
			return;
		}

		NativeScriptsComponent::NativeScript& targetScript = nativeScripts.scripts[index];

		targetScript.Reset();

		nativeScripts.scripts.erase(nativeScripts.scripts.begin() + index);

		if (removeComponentIfEmpty && nativeScripts.scripts.empty()) RemoveComponent(entity, NativeScriptsComponentType);
	}

	void EntityManager::SwapScripts(Entity entity, std::size_t a, std::size_t b) {
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			Logger::Get().Info(DebugType::GameManager) << "SwapScripts: Entity does not have any scripts!" << std::endl;
			return;
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		if (a >= nativeScripts.scripts.size() || b >= nativeScripts.scripts.size() || a < 0 || b < 0) {
			Logger::Get().Info(DebugType::GameManager) << "SwapScripts: Invalid Index used for swapping!" << std::endl;
			return;
		}

		std::swap(nativeScripts.scripts[a], nativeScripts.scripts[b]);
	}

	std::vector<Entity> EntityManager::GetAllEntities() const {
		std::vector<Entity> entities;
		entities.reserve(ecs.entitySignatures.size());
		for (const auto& pair : ecs.entitySignatures) {
			entities.push_back(pair.first);
		}
		return entities;
	}

	std::vector<Entity> EntityManager::GetAllEntitiesWithComponent(ComponentType componentType) const {
		std::vector<Entity> entities;
		for (const auto& [entity, signature] : ecs.entitySignatures) {
			if (signature.test(componentType)) {
				entities.push_back(entity);
			}
		}
		return entities;
	}

#pragma endregion

#pragma region WIP ENTITYMANAGER

	// ---------------------------------------------------------------------------
	// Constructors
	// ---------------------------------------------------------------------------
	WIPEntityManager::WIPEntityManager(WIPEntityStorage& entityStorage) : ecs(entityStorage) {};

	// ---------------------------------------------------------------------------
	// EntityManager APIs
	// ---------------------------------------------------------------------------
	Entity WIPEntityManager::CreateEntity() {
		using Archetype = WIPEntityStorage::Archetype;

		ComponentSignature emptySignature; // New entity starts with no components
		Entity newEntity = nextEntityID++;

		size_t archetypeIndex = ecs.archetypes.size();
		for (size_t i = 0; i < ecs.archetypes.size(); ++i) {
			// If an archetype with an empty signature exists, use it
			if (ecs.archetypes[i].signature.none()) {
				archetypeIndex = i;
				break;
			}
		}

		// If there is no empty archetype, create one
		if (archetypeIndex == ecs.archetypes.size()) {
			Archetype newArch;
			newArch.signature = emptySignature;
			ecs.archetypes.push_back(newArch);
		}

		Archetype& emptyArchetype = ecs.archetypes[archetypeIndex];
		emptyArchetype.entities.push_back(newEntity);

		ecs.entityToArchetype[newEntity] = archetypeIndex;
		ecs.entitySignatures[newEntity] = emptySignature;

		return newEntity;
	}

	Entity WIPEntityManager::CloneEntity(/*Entity& entity*/) {
		using Archetype = WIPEntityStorage::Archetype;

		ComponentSignature otherSignature;
		Entity newEntity = nextEntityID++;

		return newEntity;
	}

	bool WIPEntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		auto it = ecs.entitySignatures.find(entity);
		if (it != ecs.entitySignatures.end()) {
			const ComponentSignature& signature = it->second;
			return signature.test(componentType);
		}

		return false; // Entity not found or does not have the component
	}

	/*void WIPEntityManager::AddComponent(Entity entity, ComponentType componentType) {
		// Empty by design.
	}*/

#pragma endregion
}