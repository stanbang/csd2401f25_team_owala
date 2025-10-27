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
#include "EntityManager.h"
#include "Logger.h"
#include "EntitySerializer.h"

namespace Framework {
	using namespace SceneManagement;

	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	SceneManager* SceneManager::instance = nullptr; // Initialize the static instance pointer to nullptr
	SceneGUID SceneManager::nextGUID = 1; // Start from 1, 0 is reserved for invalid scene. This is set during compile time.
	const std::string INVALID_SCENENAME = "InvalidScene"; // Reserved name for invalid scene

	// Constructor
	SceneManager::SceneManager() {
		invalidScene.sceneIndex = 0;
		invalidScene.name = INVALID_SCENENAME;
		invalidScene.path = {};
		invalidScene.isLoaded = false;
	}


	SceneManager& SceneManager::GetInstance() {
		if (!instance) {
			instance = new SceneManager();
			Logger::Get().Info(DebugType::SceneManager) << "Scene System Instantiated!" << std::endl;
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
	// SceneManager Internals
	// ---------------------------------------------------------------------------

	SceneAsset* SceneManager::GetSceneAssetMutable(SceneGUID sceneIndex) {
		auto it = scenes_.find(sceneIndex);
		if (it != scenes_.end()) {
			return &(it->second);
		}

		return nullptr; // Return nullptr if scene not found
	}

	// ---------------------------------------------------------------------------
	// SceneManager Configuration
	// ---------------------------------------------------------------------------
	void SceneManager::SetEntityStorageRef(EntityStorage& entityStorageRef) {
		entityStorageRef_ = &entityStorageRef;
		Logger::Get().Info(DebugType::SceneManager) << "Successfully linked to Entity Storage" << std::endl;

	}

	// ---------------------------------------------------------------------------
	// Scene Management APIs
	// ---------------------------------------------------------------------------
	#pragma region Scene Asset Stuff
	SceneGUID SceneManager::CreateSceneAsset(const std::string& sceneName, const std::string& path) {
		CheckStorage();
		// Check if scene with the same name already exists
		for (const auto& [id, scene] : scenes_) {
			if (scene.name == sceneName) {
				Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' already exists. Creation failed." << std::endl;
				return 0;
			}
		}

		SceneAsset newScene;
		newScene.sceneIndex = nextGUID++;
		newScene.name = sceneName;
		newScene.path = path;
		scenes_[newScene.sceneIndex] = newScene;
		Logger::Get().Info(DebugType::SceneManager) << "Created new Scene: " << sceneName << " with ID: " << newScene.sceneIndex << std::endl;
		return newScene.sceneIndex;
	}

	const SceneAsset& SceneManager::GetSceneByName(const std::string& sceneName, bool silenceError) const {
		for (const auto& [id, scene] : scenes_) {
			if (scene.name == sceneName) {
				return scene;
			}
		}

		if (!silenceError)
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist." << std::endl;
		return invalidScene;
	}

	const SceneAsset& SceneManager::GetSceneAt(SceneGUID sceneIndex, bool silenceError) const {
		for (const auto& [id, scene] : scenes_) {
			if (scene.sceneIndex == sceneIndex) {
				return scene;
			}
		}

		if (!silenceError)
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist." << std::endl;
		return invalidScene;
	}

	const SceneAsset& SceneManager::GetActiveScene() const {
		return GetSceneAt(currentScene_);
	}

	void SceneManager::LoadScene(const std::string& sceneName) {
		SceneAsset sceneToLoad = GetSceneByName(sceneName);
		if (!sceneToLoad.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		UnloadScene(); // Unload current active scene first if any

		EntityManager entityMgr(*entityStorageRef_);
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			SceneOwned sceneOwned = entityMgr.GetComponent<SceneOwned>(entity);
			if (sceneOwned.sceneName == sceneName) {
				entityMgr.AddComponent(entity, ActiveTag{});
				//entityStorageRef_->activeTag[entity] = ActiveTag{};
			}
			else if (entityMgr.HasComponent(entity, PersistentTagType)) { // Checks if entity has a scene component and is persistent
				entityMgr.AddComponent(entity, ActiveTag{});
				//entityStorageRef_->activeTag[entity] = ActiveTag{}; // If entity is persistent and belongs to another scene, it should still be loaded
			}
		}

		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " << sceneName << " with GUID: " << sceneToLoad.sceneIndex << std::endl;
		currentScene_ = sceneToLoad.sceneIndex;
	}

	void SceneManager::LoadScene(const SceneGUID sceneIndex) {
		SceneAsset sceneToLoad = GetSceneAt(sceneIndex);
		if (!sceneToLoad.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		UnloadScene(); // Unload current active scene first if any

		EntityManager entityMgr(*entityStorageRef_);
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			SceneOwned sceneOwned = entityMgr.GetComponent<SceneOwned>(entity);
			if (sceneOwned.sceneName == sceneToLoad.name) {
				entityMgr.AddComponent(entity, ActiveTag{});
				//entityStorageRef_->activeTag[entity] = ActiveTag{};
			}
			else if (entityMgr.HasComponent(entity, PersistentTagType)) { // Checks if entity has a scene component and is persistent
				entityMgr.AddComponent(entity, ActiveTag{});
				//entityStorageRef_->activeTag[entity] = ActiveTag{}; // If entity is persistent and belongs to another scene, it should still be loaded
			}
		}

		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " << sceneToLoad.name << " with GUID: " << sceneToLoad.sceneIndex << std::endl;
		currentScene_ = sceneToLoad.sceneIndex;
		scenes_.at(currentScene_).isLoaded = true;
	}

	void SceneManager::LoadScene(const SceneAsset& scene) {
		LoadScene(scene.sceneIndex);
	}

	void SceneManager::UnloadScene() {
		CheckStorage();

		if (currentScene_ == 0) return; // No scene is currently loaded

		EntityManager entityMgr(*entityStorageRef_);
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ActiveTagType)) {
			entityMgr.RemoveComponent(entity, ActiveTagType);
		}

		GetSceneAssetMutable(currentScene_)->isLoaded = false;
		prevScene_ = currentScene_;
		currentScene_ = 0;

		// For now it just hides the entities. Let me know if you want the entities to be destroyed instead. 
		// By destroy I mean remove all components and tags associated with the entity and render their IDs stale.
		// This is applicable to all UnloadScene Functions and their overloads.
	}

	#pragma endregion

	#pragma region Entity Scene Stuff
	void SceneManager::AddEntityToScene(const std::string& sceneName, Entity entity) {
		CheckStorage();

		SceneAsset targetScene = GetSceneByName(sceneName);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << " Scene with name '" << sceneName << "' does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		EntityManager entityMgr(*entityStorageRef_);
		entityMgr.AddComponent(entity, SceneOwned{ targetScene.name });
		Logger::Get().Info(DebugType::SceneManager) << "Added Entity[" << entity << "] to Scene [" << targetScene.name << "]" << std::endl;

		// Immediately mark the entity to be displayed if the scene is currently loaded
		if (targetScene.isLoaded) {
			entityMgr.AddComponent(entity, ActiveTag{});
		}
	}

	void SceneManager::AddEntityToScene(const SceneGUID sceneIndex, Entity entity) {
		CheckStorage();
		SceneAsset targetScene = GetSceneAt(sceneIndex);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Cannot add Entity[" << entity + "] to it." << std::endl;
			return;
		}

		EntityManager entityMgr(*entityStorageRef_);
		entityMgr.AddComponent(entity, SceneOwned{ targetScene.name });
		Logger::Get().Info(DebugType::SceneManager) << "Added Entity[" << entity << "] to Scene [" << targetScene.name << "]" << std::endl;

		// Immediately mark the entity to be displayed if the scene is currently loaded
		if (targetScene.isLoaded) {
			entityMgr.AddComponent(entity, ActiveTag{});
		}
	}

	void SceneManager::AddEntityToScene(const SceneAsset& scene, Entity entity) {
		CheckStorage();
		SceneAsset targetScene = GetSceneAt(scene.sceneIndex);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << targetScene.name << "' does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		EntityManager entityMgr(*entityStorageRef_);
		entityMgr.AddComponent(entity, SceneOwned{ targetScene.name });
		Logger::Get().Info(DebugType::SceneManager) << "Added Entity[" << entity << "] to Scene [" << targetScene.name + "]" << std::endl;

		// Immediately mark the entity to be displayed if the scene is currently loaded
		if (targetScene.isLoaded) {
			entityMgr.AddComponent(entity, ActiveTag{});
		}
	}

	void SceneManager::RemoveEntityFromScene(Entity entity) {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		entityMgr.RemoveComponent(entity, SceneOwnedType);
		entityMgr.RemoveComponent(entity, ActiveTagType);
		
		Logger::Get().Info(DebugType::SceneManager) << "Removed Entity[" << entity << "] from its Scene" << std::endl;
	}

	void SceneManager::SetEntityAsPersistent(Entity entity, bool shouldPersist) {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		if (shouldPersist) {
			entityMgr.AddComponent(entity, PersistentTag{});
		}
		else {
			entityMgr.RemoveComponent(entity, PersistentTagType);
		}
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(const std::string& scene) const {
		(void)scene; // Suppress unused parameter warning for now
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);

		return entityMgr.GetAllEntitiesWithComponent(ActiveTagType);
	}

	#pragma endregion

	// ---------------------------------------------------------------------------
	// DEBUG FUNCTIONS
	// ---------------------------------------------------------------------------
	void SceneManager::PrintAllScenesInfo() const {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		std::unordered_map<std::string, std::vector<Entity>> scenes;
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(SceneOwnedType)) {
			SceneOwned sceneOwned = entityMgr.GetComponent<SceneOwned>(entity);
			scenes[sceneOwned.sceneName].push_back(entity);
		}

		Logger::Get().Info(DebugType::SceneManager) << "------- All Scenes Info ------" << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Total Scenes: " << scenes.size() << std::endl;
		
		for (const auto& [sceneName, entities] : scenes) {
			size_t persistentCount = 0;

			// Count how many entities in this scene are persistent
			for (const Entity e : entities) {
				if (entityMgr.HasComponent(e, PersistentTagType)) {
					++persistentCount;
				}
			}

			Logger::Get().Info(DebugType::SceneManager) << "Scene: " << sceneName << " | Entities: " << entities.size() << ", " << persistentCount << " Persistent" << std::endl;
		}

		Logger::Get().Info(DebugType::SceneManager) << "------------------------------" << std::endl;

		// Will add more detailed info later if needed
	}

	void SceneManager::PrintCurrentSceneInfo() const {
		CheckStorage();

		EntityManager entityMgr(*entityStorageRef_);
		if (!GetSceneAt(currentScene_, true).IsValid()) {
			Logger::Get().Info(DebugType::SceneManager) << "No active scene currently loaded." << std::endl;
			return;
		}
		
		Logger::Get().Info(DebugType::SceneManager) << "----- Current Scene Info -----" << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " + GetSceneAt(currentScene_).name << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Active Entities Count: " + entityMgr.GetAllEntitiesWithComponent(ActiveTagType).size() << std::endl;

		std::ostream& logStream = Logger::Get().Info(DebugType::SceneManager);
		logStream << "Active Entities IDs: ";
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ActiveTagType)) {
			logStream << "Entity[" << entity << "]";
			if (entityMgr.HasComponent(entity, PersistentTagType)) {
				logStream << " (Persistent)";
			}
			logStream << ", ";
		}
		logStream << std::endl;
		
		Logger::Get().Info(DebugType::SceneManager) << "------------------------------" << std::endl;
	}
}