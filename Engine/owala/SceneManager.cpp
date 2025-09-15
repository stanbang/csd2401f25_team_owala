/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing scenes/levels
 *
 *
 * \author	Jason Lou
 * \date	11 September 2025
 *********************************************************************/
#include "SceneManager.h"
#include "EntityStorage.h"
#include <iostream>

namespace Framework {
	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	SceneManager* SceneManager::instance = nullptr; // Initialize the static instance pointer to nullptr

	SceneManager& SceneManager::GetInstance() {
		if (!instance) {
			instance = new SceneManager();
			std::cout << "[SceneManager] Scene System Instantiated!" << std::endl;
		}
		return *instance;
	}

	bool SceneManager::Exists() {
		return instance != nullptr;
	}

	void SceneManager::DestroyInstance() {
		delete instance;
		instance = nullptr;
	}

	// ---------------------------------------------------------------------------
	// SceneManager Checkers
	// ---------------------------------------------------------------------------
	void SceneManager::CheckStorage() {
		if (!entityStorageRef_) {
			throw std::runtime_error("Entity Storage must be set before use!");
		}
	}

	void SceneManager::CheckStorage() const {
		if (!entityStorageRef_) {
			throw std::runtime_error("Entity Storage must be set before use!");
		}
	}

	// ---------------------------------------------------------------------------
	// SceneManager Configuration
	// ---------------------------------------------------------------------------
	void SceneManager::SetEntityStorageRef(EntityStorage& entityStorageRef) {
		entityStorageRef_ = &entityStorageRef;
		std::cout << "[SceneManager] Successfully linked to Entity Storage" << std::endl;
	}

	// ---------------------------------------------------------------------------
	// Scene Management APIs
	// ---------------------------------------------------------------------------
	void SceneManager::AddEntityToScene(const std::string& scene, Entity entity) {
		CheckStorage();

		entityStorageRef_->sceneOwned[entity] = SceneOwned{ scene };
		std::cout << "[SceneManager] Added Entity[" << entity << "] to Scene [" << scene << "]" << std::endl;
	}

	void SceneManager::RemoveEntityFromScene(Entity entity) {
		CheckStorage();

		entityStorageRef_->sceneOwned.erase(entity);
		entityStorageRef_->activeTag.erase(entity);
		std::cout << "[SceneManager] Removed Entity[" << entity << "] from its Scene" << std::endl;
	}

	void SceneManager::SetEntityAsPersistent(Entity entity, bool shouldPersist) {
		CheckStorage();

		if (shouldPersist) {
			entityStorageRef_->persistentTag[entity] = PersistentTag{};
		}
		else {
			entityStorageRef_->persistentTag.erase(entity);
		}
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(const std::string& scene) const {
		CheckStorage();

		std::vector<Entity> entitiesInScene;
		for (const auto& [entity, activeTag] : entityStorageRef_->activeTag) {
			entitiesInScene.push_back(entity);
		}
		return entitiesInScene;
	}

	void SceneManager::LoadScene(const std::string& scene) {
		UnloadScene(activeScene); // Unload current active scene first if any
		
		for(const auto& [entity, sceneOwned] : entityStorageRef_->sceneOwned) {
			if(sceneOwned.sceneName == scene) {
				entityStorageRef_->activeTag[entity] = ActiveTag{};
			}
			else if (entityStorageRef_->persistentTag.find(entity) != entityStorageRef_->persistentTag.end()) { // Checks if entity has a scene component and is persistent
				entityStorageRef_->activeTag[entity] = ActiveTag{}; // If entity is persistent and belongs to another scene, it should still be loaded
			}
		}

		activeScene = scene;
	}

	void SceneManager::UnloadScene(const std::string& scene) {
		CheckStorage();

		entityStorageRef_->activeTag.clear();
		activeScene = "";

		// For now it just hides the entities. Let me know if you want the entities to be destroyed instead. 
		// By destroy I mean remove all components and tags associated with the entity and render their IDs stale.
	}

	bool SceneManager::IsSceneLoaded(const std::string& scene) const {
		CheckStorage();

		return activeScene == scene;
	}
}