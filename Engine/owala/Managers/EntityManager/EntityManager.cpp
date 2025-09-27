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

		return newEntity;
	}

	Entity EntityManager::CloneEntity(Entity& entity) {
		if (ecs.entitySignatures.find(entity) == ecs.entitySignatures.end()) {
			throw std::runtime_error("Attempted to clone a non existent entity!");
		}

		Entity newEntity = CreateEntity();
		ComponentSignature otherSignature = ecs.entitySignatures[entity];
		ecs.entitySignatures[newEntity] = otherSignature;

		if (otherSignature[PositionType])     ecs.position[newEntity] = ecs.position[entity];
		if (otherSignature[VelocityType])     ecs.velocity[newEntity] = ecs.velocity[entity];
		if (otherSignature[AccelerationType]) ecs.acceleration[newEntity] = ecs.acceleration[entity];
		if (otherSignature[RigidBodyType])    ecs.rigidBody[newEntity] = ecs.rigidBody[entity];
		if (otherSignature[AABBColliderType]) ecs.colliders[newEntity] = ecs.colliders[entity];
		if (otherSignature[TriangleType])     ecs.triangle[newEntity] = ecs.triangle[entity];
		if (otherSignature[TextureComponentType]) ecs.tex[newEntity] = ecs.tex[entity];
		if (otherSignature[PlayerStatsType])  ecs.playerStats[newEntity] = ecs.playerStats[entity];
		if (otherSignature[PlayerStateType])  ecs.playerState[newEntity] = ecs.playerState[entity];
		if (otherSignature[RotationFlagType]) ecs.rotationFlag[newEntity] = ecs.rotationFlag[entity];

		return newEntity;
	}

	bool EntityManager::HasComponent(Entity entity, ComponentType componentType) const {
		auto it = ecs.entitySignatures.find(entity);
		if (it != ecs.entitySignatures.end()) {
			const ComponentSignature& signature = it->second;
			return signature.test(componentType);
		}

		return false; // Entity not found or does not have the component
	}

	//template <typename T>
	//void EntityManager::AddComponent(Entity entity, const T& component) {
	//	// Get the component type from the trait
	//	constexpr ComponentType type = ComponentTypeTrait<T>::value;

	//	if (HasComponent(entity, type)) {
	//		return; // Component already exists, do nothing
	//	}

	//	// Store the component in the appropriate map
	//	if constexpr (std::is_same_v<T, Position>) ecs.position[entity] = component;
	//	else if constexpr (std::is_same_v<T, Velocity>) ecs.velocity[entity] = component;
	//	else if constexpr (std::is_same_v<T, Acceleration>) ecs.acceleration[entity] = component;
	//	else if constexpr (std::is_same_v<T, RigidBody>) ecs.rigidBody[entity] = component;
	//	else if constexpr (std::is_same_v<T, AABBCollider>) ecs.colliders[entity] = component;
	//	else if constexpr (std::is_same_v<T, Triangle>) ecs.triangle[entity] = component;
	//	else if constexpr (std::is_same_v<T, TextureComponent>) ecs.tex[entity] = component;
	//	else if constexpr (std::is_same_v<T, PlayerStats>) ecs.playerStats[entity] = component;
	//	else if constexpr (std::is_same_v<T, PlayerState>) ecs.playerState[entity] = component;
	//	else if constexpr (std::is_same_v<T, RotationFlag>) ecs.rotationFlag[entity] = component;
	//	else if constexpr (std::is_same_v<T, SceneOwned>) ecs.sceneOwned[entity] = component;
	//	else {
	//		static_assert(!std::is_same_v<T, T>, "Unsupported component type");
	//	}

	//	// Set the bit in the entity signature
	//	ecs.entitySignatures[entity].set(type);
	//}

	void EntityManager::RemoveComponent(Entity entity, ComponentType componentType) {
		if (!HasComponent(entity, componentType)) return;

		switch (componentType) {
			case PositionType:      ecs.position.erase(entity); break;
			case VelocityType:      ecs.velocity.erase(entity); break;
			case AccelerationType:  ecs.acceleration.erase(entity); break;
			case RigidBodyType:     ecs.rigidBody.erase(entity); break;
			case AABBColliderType:  ecs.colliders.erase(entity); break;
			case TriangleType:      ecs.triangle.erase(entity); break;
			case TextureComponentType: ecs.tex.erase(entity); break;
			case PlayerStatsType:   ecs.playerStats.erase(entity); break;
			case PlayerStateType:   ecs.playerState.erase(entity); break;
			case RotationFlagType:  ecs.rotationFlag.erase(entity); break;
			case SceneOwnedType: ecs.sceneOwned.erase(entity); break;
			default: break;
		}

		ecs.entitySignatures[entity].reset(componentType);
	}

	//template <typename T>
	//T& EntityManager::GetComponent(Entity entity) {
	//	// Get the component type from the trait
	//	constexpr ComponentType type = ComponentTypeTrait<T>::value;

	//	// Ensure the entity actually has this component
	//	if (!HasComponent(entity, type)) {
	//		throw std::runtime_error("GetComponent: Entity does not have the requested component");
	//	}

	//	// Return a reference to the component in the map
	//	if constexpr (std::is_same_v<T, Position>)      return ecs.position.at(entity);
	//	else if constexpr (std::is_same_v<T, Velocity>) return ecs.velocity.at(entity);
	//	else if constexpr (std::is_same_v<T, Acceleration>) return ecs.acceleration.at(entity);
	//	else if constexpr (std::is_same_v<T, RigidBody>) return ecs.rigidBody.at(entity);
	//	else if constexpr (std::is_same_v<T, AABBCollider>) return ecs.colliders.at(entity);
	//	else if constexpr (std::is_same_v<T, Triangle>) return ecs.triangle.at(entity);
	//	else if constexpr (std::is_same_v<T, TextureComponent>) return ecs.tex.at(entity);
	//	else if constexpr (std::is_same_v<T, PlayerStats>) return ecs.playerStats.at(entity);
	//	else if constexpr (std::is_same_v<T, PlayerState>) return ecs.playerState.at(entity);
	//	else if constexpr (std::is_same_v<T, RotationFlag>) return ecs.rotationFlag.at(entity);
	//	else {
	//		static_assert(!std::is_same_v<T, T>, "Unsupported component type");
	//	}
	//}

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

	Entity WIPEntityManager::CloneEntity(Entity& entity) {
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

	void WIPEntityManager::AddComponent(Entity entity, ComponentType componentType) {
		// Empty by design.
	}

#pragma endregion
}