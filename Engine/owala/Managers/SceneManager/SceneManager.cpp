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
#include "Logger.h"

namespace Framework {
	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	SceneManager* SceneManager::instance = nullptr; // Initialize the static instance pointer to nullptr

	SceneManager& SceneManager::GetInstance() {
		if (!instance) {
			instance = new SceneManager();
			Logger logger;
			logger.Info(DebugType::SceneManager, "Scene System Instantiated!");
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
			throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
		}
	}

	void SceneManager::CheckStorage() const {
		if (!entityStorageRef_) {
			throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
		}
	}

	// ---------------------------------------------------------------------------
	// SceneManager Configuration
	// ---------------------------------------------------------------------------
	void SceneManager::SetEntityStorageRef(EntityStorage& entityStorageRef) {
		entityStorageRef_ = &entityStorageRef;
		Logger logger;
		logger.Info(DebugType::SceneManager, "Successfully linked to Entity Storage");

	}

	// ---------------------------------------------------------------------------
	// Scene Management APIs
	// ---------------------------------------------------------------------------
	void SceneManager::AddEntityToScene(const std::string& scene, Entity entity) {
		CheckStorage();

		entityStorageRef_->sceneOwned[entity] = SceneOwned{ scene };
		Logger logger;
		std::string logMsg{};
		logMsg = format("Added Entity [{}] to Scene [{}]", entity, scene);
		logger.Info(DebugType::SceneManager, logMsg);
	}

	void SceneManager::RemoveEntityFromScene(Entity entity) {
		CheckStorage();

		entityStorageRef_->sceneOwned.erase(entity);
		entityStorageRef_->activeTag.erase(entity);
		Logger logger;
		std::string logMsg{};
		logMsg = format("Removed Entity[{}] from its Scene", entity);
		logger.Info(DebugType::SceneManager, logMsg);
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

	// ---------------------------------------------------------------------------
	// DEBUG FUNCTIONS
	// ---------------------------------------------------------------------------
	void SceneManager::PrintAllScenesInfo() const {
		CheckStorage();

		std::unordered_map<std::string, std::vector<Entity>> scenes;
		for (const auto& [entity, sceneOwned] : entityStorageRef_->sceneOwned) {
			scenes[sceneOwned.sceneName].push_back(entity);
		}

		Logger logger;
		logger.Info(DebugType::SceneManager, "------- All Scenes Info ------");
		logger.Info(DebugType::SceneManager, "Total Scenes: " + scenes.size());
		
		for (const auto& [sceneName, entities] : scenes) {
			size_t persistentCount = 0;

			// Count how many entities in this scene are persistent
			for (Entity e : entities) {
				if (entityStorageRef_->persistentTag.find(e) != entityStorageRef_->persistentTag.end()) {
					++persistentCount;
				}
			}

			std::string logMsg{};
			logMsg = format("Scene: {} | Entities: {}, {} Persistent", sceneName, entities.size(), persistentCount);
			logger.Info(DebugType::SceneManager, logMsg);
		}

		logger.Info(DebugType::SceneManager, "------------------------------");

		// Will add more detailed info later if needed
	}

	void SceneManager::PrintCurrentSceneInfo() const {
		CheckStorage();
		
		Logger logger;
		logger.Info(DebugType::SceneManager, "----- Current Scene Info -----");
		logger.Info(DebugType::SceneManager, "Loaded Scene: " + activeScene);
		logger.Info(DebugType::SceneManager, "Active Entities Count: " + entityStorageRef_->activeTag.size());

		std::ostringstream oss;
		oss << "Active Entities IDs: ";
		for (const auto& [entity, activeTag] : entityStorageRef_->activeTag) {
			oss << "Entity[" << entity << "]";
			if (entityStorageRef_->persistentTag.find(entity) != entityStorageRef_->persistentTag.end()) {
				oss << " (Persistent)";
			}
			oss << ", ";
		}

		std::string logMsg = oss.str();
		logger.Info(DebugType::SceneManager, logMsg);
		logger.Info(DebugType::SceneManager, "------------------------------");
	}
}