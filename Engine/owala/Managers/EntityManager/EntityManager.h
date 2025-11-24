/*****************************************************************//**
* \file   EntityManager.h
*
* \primary_author
*      Jason Lou
* \secondary_author
*
*
* \brief	Manager responsible for managing entities and their
*			components in the ECS architecture. Also serves as a
*			Entity Factory/Game Object Factory to create entities.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "EntityStorage.h"
#include <stdexcept>
#include <bitset>

namespace Framework {

	// THIS CLASS IS A UTILITY CLASS. Its not meant to store any data. Use it for API calls only.
	class EntityManager {
	private:
		EntityStorage& ecs;
	public:
		/* Constructors */
		EntityManager(EntityStorage& entityStorage);

		/* EntityManager APIs */
		// Creates a new entity and returns its ID.
		Entity CreateEntity();
		// Creates a new entity with the specified ID.
		// Use with caution, this will overwrite any existing entity with conflicting ID.
		Entity CreateEntityWithID(Entity entityID);
		// Clones the specified entity, creating a new entity with the same components and data, and returns the new entity's ID.
		Entity CloneEntity(const Entity& entity);
		// Destroys the specified entity, the entity and all its components are destroyed together.
		void DestroyEntity(Entity entity);

		// Gets the property data of the specified entity.
		EntityProperties& GetProperties(Entity entity);

		// Validate if Entity exists. Returns true if Entity is a valid entity.
		bool ValidateEntity(Entity entity);

		// Checks if the specified entity has the specified component type attached to it. Returns true if the component is present, false otherwise.
		bool HasComponent(Entity entity, ComponentType componentType) const;
		// Adds a component of type T to the specified entity. The Component must be a valid component type defined in Components.h
		template <typename T>
		void AddComponent(Entity entity, const T& component) {
			constexpr ComponentType type = ComponentTypeTrait<T>::value;

			if (HasComponent(entity, type)) return;

			if constexpr (std::is_same_v<T, Position>) ecs.position[entity] = component;
			else if constexpr (std::is_same_v<T, SpawnPoint>) ecs.spawnPoint[entity] = component;
			else if constexpr (std::is_same_v<T, Velocity>) ecs.velocity[entity] = component;
			else if constexpr (std::is_same_v<T, Acceleration>) ecs.acceleration[entity] = component;
			else if constexpr (std::is_same_v<T, Triangle>)  ecs.triangle[entity] = component;
			else if constexpr (std::is_same_v<T, RigidBody>) ecs.rigidBody[entity] = component;
			else if constexpr (std::is_same_v<T, AABBCollider>) ecs.colliders[entity] = component;
			else if constexpr (std::is_same_v<T, PlayerState>) ecs.playerState[entity] = component;
			else if constexpr (std::is_same_v<T, RotationFlag>) ecs.rotationFlag[entity] = component;
			else if constexpr (std::is_same_v<T, TrapPlatformComponent>) ecs.trapPlatform[entity] = component;
			else if constexpr (std::is_same_v<T, TextureComponent>) ecs.tex[entity] = component;
			else if constexpr (std::is_same_v<T, AudioComponent>) ecs.audio[entity] = component;
			else if constexpr (std::is_same_v<T, AnimationComponent>) ecs.animatedTex[entity] = component;
			else if constexpr (std::is_same_v<T, TextComponent>) ecs.text[entity] = component;
			else if constexpr (std::is_same_v<T, Button>) ecs.button[entity] = component;
			else if constexpr (std::is_same_v<T, SceneOwned>) ecs.sceneOwned[entity] = component;
			else if constexpr (std::is_same_v<T, Camera>) ecs.cameras[entity] = component;
			else if constexpr (std::is_same_v<T, NativeScriptsComponent>) ecs.nativeScripts[entity] = component;
			else if constexpr (std::is_same_v<T, BirdAIComponent>) ecs.birdAIComponent[entity] = component;
			else if constexpr (std::is_same_v<T, SkeletonAIComponent>) ecs.skeletonAIComponent[entity] = component;
			else if constexpr (std::is_same_v<T, ParticleComponent>) ecs.particleComponent[entity] = component;
			else if constexpr (std::is_same_v<T, ParticleEmitterComponent>) ecs.particleEmitterComponent[entity] = component;
			else if constexpr (std::is_same_v<T, Rotation>) ecs.rotation[entity] = component;
			else if constexpr (std::is_same_v<T, Scale>) ecs.scale[entity] = component;
			else {
				// If this is a tag-only type, no storage is necessary.
				static_assert(std::is_same_v<T, T>, "Unsupported component type"); // will never be triggered when all types are handled
			}

			ecs.entitySignatures[entity].set(type);
		}
		// Adds a empty component of specified ComponentType ID to the entity.
		void AddComponent(Entity entity, ComponentType componentType);
		// Removes a component of type T from the specified entity. The Component must be a valid component type defined in Components.h
		void RemoveComponent(Entity entity, ComponentType componentType);
		// Gets the component of type T attached to the specified entity. Throws an error if the entity does not have the component.
		template <typename T>
		T& GetComponent(Entity entity) {
			// Get the component type from the trait
			constexpr ComponentType type = ComponentTypeTrait<T>::value;

			// Ensure the entity actually has this component
			if (!HasComponent(entity, type)) {
				throw std::runtime_error("GetComponent: Entity does not have the requested component");
			}

			// Return a reference to the component in the map
			if constexpr (std::is_same_v<T, Position>) return ecs.position.at(entity);
			else if constexpr (std::is_same_v<T, SpawnPoint>) return ecs.spawnPoint.at(entity);
			else if constexpr (std::is_same_v<T, Velocity>) return ecs.velocity.at(entity);
			else if constexpr (std::is_same_v<T, Acceleration>) return ecs.acceleration.at(entity);
			else if constexpr (std::is_same_v<T, Triangle>) return ecs.triangle.at(entity);
			else if constexpr (std::is_same_v<T, RigidBody>) return ecs.rigidBody.at(entity);
			else if constexpr (std::is_same_v<T, AABBCollider>) return ecs.colliders.at(entity);
			else if constexpr (std::is_same_v<T, PlayerState>) return ecs.playerState.at(entity);
			else if constexpr (std::is_same_v<T, RotationFlag>) return ecs.rotationFlag.at(entity);
			else if constexpr (std::is_same_v<T, TrapPlatformComponent>) return ecs.trapPlatform.at(entity);
			else if constexpr (std::is_same_v<T, TextureComponent>) return ecs.tex.at(entity);
			else if constexpr (std::is_same_v<T, AudioComponent>) return ecs.audio.at(entity);
			else if constexpr (std::is_same_v<T, AnimationComponent>) return ecs.animatedTex.at(entity);
			else if constexpr (std::is_same_v<T, Button>) return ecs.button.at(entity);
			else if constexpr (std::is_same_v<T, TextComponent>) return ecs.text.at(entity);
			else if constexpr (std::is_same_v<T, SceneOwned>) return ecs.sceneOwned.at(entity);
			else if constexpr (std::is_same_v<T, Camera>) return ecs.cameras.at(entity);
			else if constexpr (std::is_same_v<T, NativeScriptsComponent>) return ecs.nativeScripts.at(entity);
			else if constexpr (std::is_same_v<T, BirdAIComponent>) return ecs.birdAIComponent.at(entity);
			else if constexpr (std::is_same_v<T, SkeletonAIComponent>) return ecs.skeletonAIComponent.at(entity);
			else if constexpr (std::is_same_v<T, ParticleComponent>) return ecs.particleComponent.at(entity);
			else if constexpr (std::is_same_v<T, ParticleEmitterComponent>) return ecs.particleEmitterComponent.at(entity);
			else if constexpr (std::is_same_v<T, Rotation>) return ecs.rotation.at(entity);
			else if constexpr (std::is_same_v<T, Scale>) return ecs.scale.at(entity);
			else {
				static_assert(!std::is_same_v<T, T>, "Unsupported component type");
			}
		}
		// Checks if given component is a Tag Component. Returns false if it is a regular component.
		bool IsTagComponent(ComponentType componentType);
		// Clears all tag components from the specified entity.
		void ClearTags(Entity entity);
		// Attaches a custom C++ script to an entity.
		template<class T> 
		std::size_t AttachScript(Entity entity, const char* displayName = nullptr, bool enabled = true);
		// Attaches a custom C++ script to an entity.
		std::size_t AttachScriptByName(Entity entity, std::string typeName, bool enabled = true);
		// Removes a custom C++ script at the provided script index from an entity.
		void RemoveScriptAt(Entity entity, std::size_t index, bool removeComponentIfEmpty = true);
		// Swaps the index of an existing script with another bounded script.
		void SwapScripts(Entity entity, std::size_t a, std::size_t b);
		// Gets all entities that have been created. Note that this includes entities that have no components.
		std::vector<Entity> GetAllEntities() const;
		// Gets all entities that have the specified component type attached to them.
		std::vector<Entity> GetAllEntitiesWithComponent(ComponentType componentType) const;
		// Gets all entities that have all the specified component types attached to them. (Multiple component support)
		template<typename... Args>
		std::vector<Entity> GetAllEntitiesWithComponents(Args... componentTypes) const;
		// Get entity signatures
		inline const auto& GetEntitySignatures() const {
			return ecs.entitySignatures;
		}
	};

	// BELOW IS STILL WIP. DO NOT USE.
	class WIPEntityManager {
	private:
		WIPEntityStorage& ecs;
	public:
		/* Constructors */
		WIPEntityManager(WIPEntityStorage& entityStorage);

		/* EntityManager APIs */
		// Creates a new entity and returns its ID.
		Entity CreateEntity();
		// Clones the specified entity, creating a new entity with the same components and data, and returns the new entity's ID.
		Entity CloneEntity(/*Entity& entity*/);

		// Checks if the specified entity has the specified component type attached to it. Returns true if the component is present, false otherwise.
		bool HasComponent(Entity entity, ComponentType componentType) const;
		// Adds a component of type T to the specified entity. The Component must be a valid component type defined in Components.h
		//template <typename T>
		//void AddComponent(Entity entity, const T& component);
		// Adds a component of the specified ComponentType to the specified entity.
		//void AddComponent(Entity entity, ComponentType componentType);
		// Removes a component of type T from the specified entity. The Component must be a valid component type defined in Components.h
		//void RemoveComponent(Entity entity, ComponentType componentType);
	};

	template<class T>
	std::size_t EntityManager::AttachScript(Entity entity, const char* displayName, bool enabled)
	{
		if (!HasComponent(entity, NativeScriptsComponentType)) {
			AddComponent<NativeScriptsComponent>(entity, NativeScriptsComponent{});
		}

		NativeScriptsComponent& nativeScripts = GetComponent<NativeScriptsComponent>(entity);

		nativeScripts.scripts.emplace_back();
		NativeScriptsComponent::NativeScript& scriptToAdd = nativeScripts.scripts.back();
		scriptToAdd.enabled = enabled;
		scriptToAdd.Bind<T>(displayName ? displayName : typeid(T).name());

		return nativeScripts.scripts.size() - 1;
	}

	template<typename... Args>
	std::vector<Entity> EntityManager::GetAllEntitiesWithComponents(Args... componentTypes) const {
		// Create the required signature bitset
		ComponentSignature requiredSignature;

		// This sets a bit for each component type passed in
		(requiredSignature.set(static_cast<size_t>(componentTypes)), ...);

		std::vector<Entity> entities;
		for (const auto& [entity, entitySignature] : ecs.entitySignatures) {

			// Check if the entity's signature includes all bits in the required signature
			if ((entitySignature & requiredSignature) == requiredSignature) {
				entities.push_back(entity);
			}
		}
		return entities;
	}
}