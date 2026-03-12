/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing scenes/levels
 *
 *
 * \author	Jason Lou
 * \date	11 September 2025
 *********************************************************************/
#include "pch.h"
#include "SceneManager.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"
#include "EntitySerializer.h"
#include "TransformationSystem.h"
#include "TimerSystem.h"  // ADD THIS INCLUDE
#include "FirebaseManager.h"  // ADD THIS INCLUDE

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
		invalidScene.sceneIndex = INVALID_SCENEID;
		invalidScene.name = INVALID_SCENENAME;
		invalidScene.path = {};
		invalidScene.isLoaded = invalidScene.isPopulated = false;

		SceneAsset ddol;
		ddol.sceneIndex = DDOL_SCENEID;
		ddol.name = DDOL_SCENENAME;
		ddol.path = {};
		ddol.isLoaded = false;
		ddol.isPopulated = true;

		scenes_.emplace(DDOL_SCENEID, ddol);
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
	//void SceneManager::CheckStorage() {
	//	if (!entityStorageRef_) {
	//		throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
	//	}
	//}

	//void SceneManager::CheckStorage() const {
	//	if (!entityStorageRef_) {
	//		throw GameException(DebugType::SceneManager, "Entity Storage must be set before use!");
	//	}
	//}

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

	void SceneManager::EnsureScenePopulated(SceneGUID sceneIndex) {
		SceneAsset* scene = GetSceneAssetMutable(sceneIndex);
		if (!scene || !scene->IsValid()) return; // Check if Scene Exists
		if (scene->isPopulated) return; // Check if scene has already been deserialized.
		if (scene->path.empty()) return; // Check if scene is deserializable from file.

		std::vector<Entity> entitiesList;
		EntitySerializer::DeserializeSceneData(GameManager::GetInstance().GetECS(), scene->path, scene->name, entitiesList);

		for (Entity entity : entitiesList) {
			AddEntityToScene(*scene, entity);
		}

		scene->isPopulated = true;
	}

	void SceneManager::RebuildLoadedTags() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(false, LoadedTagType)) {
			entityMgr.RemoveComponent(entity, LoadedTagType);
		}

		std::unordered_set<SceneGUID> loaded;
		loaded.reserve(loadedScenes_.size());
		for (SceneGUID guid : loadedScenes_) {
			loaded.insert(guid);
		}

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(false, SceneOwnedType)) {
			const SceneOwned& owned = entityMgr.GetComponent<SceneOwned>(entity);

			bool shouldBeLoaded = loaded.find(owned.sceneIndex) != loaded.end();

#ifdef BUILD_EDITOR
			if (isPlayMode_ && entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#else
			if (entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#endif
			if (shouldBeLoaded) entityMgr.AddComponent(entity, LoadedTag{});
		}
	}

#ifdef BUILD_EDITOR
	// ---------------------------------------------------------------------------
	// Scene Context Handling
	// ---------------------------------------------------------------------------
	void SceneManager::OnEnterPlay() {
		if (contextCaptured) return;

		// Capture Scene Context
		prePlayContext.scenes = scenes_;
		prePlayContext.loadedScenes = loadedScenes_;
		prePlayContext.activeScene = currentScene_;

		contextCaptured = true;
		isPlayMode_ = true;

		// Create DDOL if scene does not exist.
		if (scenes_.find(DDOL_SCENEID) == scenes_.end()) {
			SceneAsset ddol{};
			ddol.sceneIndex = DDOL_SCENEID;
			ddol.name = DDOL_SCENENAME;
			ddol.path = {};
			ddol.isLoaded = false;
			ddol.isPopulated = true;
			scenes_.emplace(DDOL_SCENEID, ddol);
		}

		if (std::find(loadedScenes_.begin(), loadedScenes_.end(), DDOL_SCENEID) == loadedScenes_.end()) {
			loadedScenes_.push_back(DDOL_SCENEID);
		}

		if (SceneAsset* ddol = GetSceneAssetMutable(DDOL_SCENEID)) {
			ddol->isLoaded = true;
		}

		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		for (Entity persistentEntity : entityMgr.GetAllEntitiesWithComponent(false, PersistentTagType)) {
			if (!entityMgr.HasComponent(persistentEntity, SceneOwnedType)) {
				entityMgr.AddComponent(persistentEntity, SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME });
			}
			else {
				entityMgr.GetComponent<SceneOwned>(persistentEntity) = SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME };
			}
		}

		RebuildLoadedTags();

		if (GameManager::Exists()) {
			if (TransformationSystem* ts = GameManager::GetInstance().GetSystems().transformation_) {
				ts->ResetGlobalRotation();
			}
		}
	}

	void SceneManager::OnExitPlay() {
		if (!contextCaptured) return;

		scenes_ = prePlayContext.scenes;
		loadedScenes_ = prePlayContext.loadedScenes;
		currentScene_ = prePlayContext.activeScene;

		contextCaptured = false;
		isPlayMode_ = false;

		// Recreate DDOL if scene does not exist.
		if (scenes_.find(DDOL_SCENEID) == scenes_.end()) {
			SceneAsset ddol{};
			ddol.sceneIndex = DDOL_SCENEID;
			ddol.name = DDOL_SCENENAME;
			ddol.path = {};
			ddol.isLoaded = false;
			ddol.isPopulated = true;
			scenes_.emplace(DDOL_SCENEID, ddol);
		}

		for (auto& [guid, scene] : scenes_) {
			scene.isLoaded = false;
		}

		for (SceneGUID guid : loadedScenes_) {
			if (SceneAsset* scene = GetSceneAssetMutable(guid)) {
				scene->isLoaded = true;
			}
		}

		// DDOL should not be loaded in edit mode.
		auto it = std::find(loadedScenes_.begin(), loadedScenes_.end(), DDOL_SCENEID);
		if (it != loadedScenes_.end()) {
			loadedScenes_.erase(it);
		}

		if (SceneAsset* ddol = GetSceneAssetMutable(DDOL_SCENEID)) {
			ddol->isLoaded = false;
		}

		RebuildLoadedTags();

		if (GameManager::Exists()) {
			if (TransformationSystem* ts = GameManager::GetInstance().GetSystems().transformation_) {
				ts->ResetGlobalRotation();
			}
		}
	}
#endif

	// ---------------------------------------------------------------------------
	// SceneManager Configuration
	// ---------------------------------------------------------------------------
	//void SceneManager::SetEntityStorageRef(EntityStorage& entityStorageRef) {
	//	entityStorageRef_ = &entityStorageRef;
	//	Logger::Get().Info(DebugType::SceneManager) << "Successfully linked to Entity Storage" << std::endl;

	//}

	// ---------------------------------------------------------------------------
	// Scene Management APIs
	// ---------------------------------------------------------------------------
#pragma region Scene Asset Stuff
	SceneGUID SceneManager::CreateSceneAsset(const std::string& sceneName, const std::string& path, const bool entitiesPopulated) {
		// Check if scene with the same name already exists
		if (CheckSceneNameExists(sceneName)) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' already exists. Creation failed." << std::endl;
			return 0;
		}

		SceneAsset newScene;
		newScene.sceneIndex = nextGUID++;
		newScene.name = sceneName;
		newScene.path = path;
		newScene.isPopulated = entitiesPopulated;
		scenes_[newScene.sceneIndex] = newScene;
		Logger::Get().Info(DebugType::SceneManager) << "Created new Scene: " << sceneName << " with ID: " << newScene.sceneIndex << std::endl;
		return newScene.sceneIndex;
	}

	const SceneAsset& SceneManager::GetSceneByName(const std::string& sceneName, bool silenceError) const {
		for (const auto& [id, scene] : scenes_) {
			if (id == DDOL_SCENEID) continue;

			if (scene.name == sceneName) {
				return scene;
			}
		}

		if (!silenceError) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist." << std::endl;
		}

		return invalidScene;
	}

	const SceneAsset& SceneManager::GetSceneAt(SceneGUID sceneIndex, bool silenceError) const {
		// Short-circuit for reserved IDs, return invalidScene without logging
		if (sceneIndex == INVALID_SCENEID || sceneIndex == DDOL_SCENEID) {
			return invalidScene;
		}

		std::unordered_map<SceneGUID, SceneAsset>::const_iterator it = scenes_.find(sceneIndex);
		if (it == scenes_.end()) {
			if (!silenceError) {
				Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist." << std::endl;
			}
			return invalidScene;
		}

		return it->second;
	}

	const SceneAsset& SceneManager::GetActiveScene() const {
		return GetSceneAt(currentScene_, true); // Silence error: no active scene is a valid state
	}

	bool SceneManager::SetActiveScene(const SceneAsset& scene) {
		SceneAsset* targetScene = GetSceneAssetMutable(scene.sceneIndex);
		if (!targetScene || !targetScene->IsValid()) {
			return false;
		}

		if (!targetScene->isLoaded) {
			return false;
		}

		if (currentScene_ == targetScene->sceneIndex) {
			return true;
		}

		currentScene_ = targetScene->sceneIndex;
		return true;
	}

	// Helper function to identify gameplay scenes
	static bool IsGameplayScene(const std::string& sceneName) {
		static const std::vector<std::string> gameplayScenes = {
			"Tutorial 1", "Tutorial 2", "Level 1", "Level 2", "Level 3", "Level 4", "Cutscene Intro"
		};

		for (const auto& scene : gameplayScenes) {
			if (sceneName == scene) return true;
		}
		return false;
	}

	void SceneManager::LoadScene(const std::string& sceneName, LoadSceneMode mode) {
		const SceneAsset& sceneToLoad = GetSceneByName(sceneName);
		if (!sceneToLoad.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with name '" << sceneName << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		LoadScene(sceneToLoad.sceneIndex, mode);
	}

	void SceneManager::LoadScene(const SceneGUID sceneIndex, LoadSceneMode mode) {
		SceneAsset* sceneToLoad = GetSceneAssetMutable(sceneIndex);
		if (!sceneToLoad || !sceneToLoad->IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Aborting Load." << std::endl;
			return;
		}

		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		// If single, unload all scenes.
		if (mode == LoadSceneMode::Single) {
			std::vector<SceneGUID> scenesToUnload;
			scenesToUnload.reserve(loadedScenes_.size());

			for (SceneGUID loadedScene : loadedScenes_) {
				if (loadedScene != DDOL_SCENEID && loadedScene != sceneIndex) {
					scenesToUnload.push_back(loadedScene);
				}
			}

			for (SceneGUID sceneToUnload : scenesToUnload) {
				UnloadScene(sceneToUnload);
			}
		}

		if (!sceneToLoad->isLoaded) {
			sceneToLoad->isLoaded = true;

			if (std::find(loadedScenes_.begin(), loadedScenes_.end(), sceneIndex) == loadedScenes_.end()) {
				loadedScenes_.push_back(sceneIndex);
			}
		}

		if (mode == LoadSceneMode::Single || currentScene_ == INVALID_SCENEID) {
			prevScene_ = currentScene_;
			currentScene_ = sceneIndex;
		}

		EnsureScenePopulated(sceneIndex);
		RebuildLoadedTags();

		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " << sceneToLoad->name << " with GUID: " << sceneToLoad->sceneIndex << std::endl;

		if (IsGameplayScene(sceneToLoad->name)) {
			// Reset any previous timer first
			// ===== TIMER INTEGRATION =====
			// Start timer for gameplay scenes
			TimerSystem::Get().Reset();

			// Start timer for this level
			TimerSystem::Get().StartTimer(sceneToLoad->name);

			Logger::Get().Info(DebugType::PlayerSystem)
				<< "[SceneManager] Timer started for gameplay scene: " << sceneToLoad->name << std::endl;
			// =============================

			MessageBus::Instance().Publish(LevelStartEvent{});
		}
	}

	void SceneManager::LoadScene(const SceneAsset& scene, LoadSceneMode mode) {
		LoadScene(scene.sceneIndex, mode);
	}

	void SceneManager::UnloadScene(const std::string& sceneName) {
		const SceneAsset& sceneToUnload = GetSceneByName(sceneName);
		if (!sceneToUnload.IsValid()) {
			Logger::Get().Info(DebugType::SceneManager) << "Scene is not loaded! Aborting unload!" << std::endl;
			return;
		}

		UnloadScene(sceneToUnload.sceneIndex);
	}

	void SceneManager::UnloadScene(SceneGUID sceneIndex) {
		// Disallow unloading of invalid/ddol scene.
		if (sceneIndex == INVALID_SCENEID || sceneIndex == DDOL_SCENEID) return;

		// Get scene name before unloading (for timer reset)
		std::string sceneName = "Unknown";
		SceneAsset* sceneToUnload = GetSceneAssetMutable(sceneIndex);
		if (sceneToUnload) {
			sceneName = sceneToUnload->name;
		}

		auto it = std::find(loadedScenes_.begin(), loadedScenes_.end(), sceneIndex);
		if (it == loadedScenes_.end()) {
			Logger::Get().Info(DebugType::SceneManager) << "Scene is not loaded! Aborting unload!" << std::endl;
			return;
		}

		// Destroy all entities belonging to scene
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		std::vector<Entity> toDestroy;
		toDestroy.reserve(64);

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(false, SceneOwnedType)) {
			if (entityMgr.GetComponent<SceneOwned>(entity).sceneIndex == sceneIndex) {
				toDestroy.push_back(entity);
			}
		}

		for (Entity entity : toDestroy) {
			entityMgr.DestroyEntity(entity);
		}

		loadedScenes_.erase(it);

		if (sceneToUnload) {
			sceneToUnload->isLoaded = false;
			sceneToUnload->isPopulated = false;
		}

		if (currentScene_ == sceneIndex) {
			currentScene_ = INVALID_SCENEID;

			for (auto rit = loadedScenes_.rbegin(); rit != loadedScenes_.rend(); ++rit) {
				if (*rit != DDOL_SCENEID) {
					currentScene_ = *rit;
					break;
				}
			}
		}

		RebuildLoadedTags();
		Logger::Get().Info(DebugType::SceneManager) << "Unloaded Scene GUID: " << sceneIndex << std::endl;

		// ===== TIMER INTEGRATION =====
		// Reset timer when unloading gameplay scenes
		if (IsGameplayScene(sceneName)) {
			TimerSystem::Get().Reset();
			Logger::Get().Info(DebugType::PlayerSystem)
				<< "[SceneManager] Timer reset for gameplay scene: " << sceneName << std::endl;
		}
		// =============================
	}

	void SceneManager::UnloadScene(const SceneAsset& scene) {
		UnloadScene(scene.sceneIndex);
	}

#pragma endregion

#pragma region Entity Scene Stuff
	void SceneManager::AddEntityToScene(const std::string& sceneName, Entity entity) {
		const SceneAsset& targetScene = GetSceneByName(sceneName);
		if (!targetScene.IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << " Scene with name '" << sceneName << "' does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		AddEntityToScene(targetScene.sceneIndex, entity);
	}

	void SceneManager::AddEntityToScene(const SceneGUID sceneIndex, Entity entity) {
		SceneAsset* targetScene = GetSceneAssetMutable(sceneIndex);
		if (!targetScene || !targetScene->IsValid()) {
			Logger::Get().Error(DebugType::SceneManager) << "Scene with ID '" << sceneIndex << "' does not exist. Cannot add Entity[" << entity << "] to it." << std::endl;
			return;
		}

		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		// Get Assigned Scene Info
		SceneGUID assignedIndex = sceneIndex;
		std::string assignedName = targetScene->name;

		// If entity is persistent, redirect it to the don't destroy on load scene
#ifdef BUILD_EDITOR
		if (isPlayMode_ && entityMgr.HasComponent(entity, PersistentTagType)) {
			assignedIndex = DDOL_SCENEID;
			assignedName = DDOL_SCENENAME;
		}
#else
		if (entityMgr.HasComponent(entity, PersistentTagType)) {
			assignedIndex = DDOL_SCENEID;
			assignedName = DDOL_SCENENAME;

			// Ensure DDOL is marked as loaded in the Engine build
			SceneAsset* ddol = GetSceneAssetMutable(DDOL_SCENEID);
			if (ddol && !ddol->isLoaded) {
				ddol->isLoaded = true;
				if (std::find(loadedScenes_.begin(), loadedScenes_.end(), DDOL_SCENEID) == loadedScenes_.end()) {
					loadedScenes_.push_back(DDOL_SCENEID);
				}
			}
		}
#endif

		// Assign Scene Ownership
		if (!entityMgr.HasComponent(entity, SceneOwnedType)) {
			entityMgr.AddComponent(entity, SceneOwned{ assignedIndex, assignedName });
		}
		else {
			entityMgr.GetComponent<SceneOwned>(entity) = SceneOwned{ assignedIndex, assignedName };
		}

		bool shouldBeLoaded = targetScene->isLoaded;

#ifdef BUILD_EDITOR
		if (isPlayMode_ && entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#else
		if (entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#endif

		if (shouldBeLoaded) {
			if (!entityMgr.HasComponent(entity, LoadedTagType)) entityMgr.AddComponent(entity, LoadedTag{});
		}
		else {
			if (entityMgr.HasComponent(entity, LoadedTagType)) entityMgr.RemoveComponent(entity, LoadedTagType);
		}

		Logger::Get().Info(DebugType::SceneManager) << "Added Entity[" << entity << "] with name " << entityMgr.GetProperties(entity).name << " to Scene[" << targetScene->name << "]" << std::endl;
	}

	void SceneManager::AddEntityToScene(const SceneAsset& scene, Entity entity) {
		AddEntityToScene(scene.sceneIndex, entity);
	}

	void SceneManager::SetEntityAsPersistent(Entity entity, bool shouldPersist) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		if (shouldPersist) {
			if (!entityMgr.HasComponent(entity, PersistentTagType)) {
				entityMgr.AddComponent(entity, PersistentTag{});
			}
#ifdef BUILD_EDITOR
			// If called during play/runtime, entity gets added to DDOL scene as well.
			if (isPlayMode_) {
				// Ensure entity has the SceneOwned Component.
				if (!entityMgr.HasComponent(entity, SceneOwnedType)) {
					entityMgr.AddComponent(entity, SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME });
				}
				else {
					entityMgr.GetComponent<SceneOwned>(entity) = SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME };
				}

				if (!entityMgr.HasComponent(entity, LoadedTagType)) {
					entityMgr.AddComponent(entity, LoadedTag{});
				}
			}
#else
			// Engine Build: Always in runtime. Always move to DDOL.
			if (!entityMgr.HasComponent(entity, SceneOwnedType)) {
				entityMgr.AddComponent(entity, SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME });
			}
			else {
				entityMgr.GetComponent<SceneOwned>(entity) = SceneOwned{ DDOL_SCENEID, DDOL_SCENENAME };
			}

			if (!entityMgr.HasComponent(entity, LoadedTagType)) {
				entityMgr.AddComponent(entity, LoadedTag{});
			}
#endif
		}
		else { // shouldPersist = false
			if (entityMgr.HasComponent(entity, PersistentTagType)) {
				entityMgr.RemoveComponent(entity, PersistentTagType);
			}
#ifdef BUILD_EDITOR
			// If persistence removed during play and currently in DDOL scene, move to current active scene.
			if (isPlayMode_ && entityMgr.HasComponent(entity, SceneOwnedType)) {
				SceneOwned& owned = entityMgr.GetComponent<SceneOwned>(entity);
				if (owned.sceneIndex == DDOL_SCENEID) {
					const SceneAsset& activeScene = GetActiveScene();
					if (activeScene.IsValid()) {
						owned.sceneIndex = activeScene.sceneIndex;
						owned.sceneName = activeScene.name;
					}
				}
			}
#endif
			bool shouldBeLoaded = false;

			if (entityMgr.HasComponent(entity, SceneOwnedType)) {
				const SceneOwned& owned = entityMgr.GetComponent<SceneOwned>(entity);
				shouldBeLoaded = std::find(loadedScenes_.begin(), loadedScenes_.end(), owned.sceneIndex) != loadedScenes_.end();
			}

#ifdef BUILD_EDITOR
			if (isPlayMode_ && entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#else
			if (entityMgr.HasComponent(entity, PersistentTagType)) shouldBeLoaded = true;
#endif

			if (shouldBeLoaded) {
				if (!entityMgr.HasComponent(entity, LoadedTagType)) entityMgr.AddComponent(entity, LoadedTag{});
			}
			else {
				if (entityMgr.HasComponent(entity, LoadedTagType)) entityMgr.RemoveComponent(entity, LoadedTagType);
			}
		}
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(const std::string& sceneName) const {
		const SceneAsset& scene = GetSceneByName(sceneName);
		if (!scene.IsValid()) {
			Logger::Get().Info(DebugType::SceneManager) << "GetEntitiesInScene: Scene is not valid!" << std::endl;
			return {};
		}

		return GetEntitiesInScene(scene.sceneIndex);
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(SceneGUID sceneIndex) const {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		std::vector<Entity> sceneEntities;

		for (Entity entity : entityMgr.GetAllEntitiesWithComponent(false, SceneOwnedType)) {
			const SceneOwned& owned = entityMgr.GetComponent<SceneOwned>(entity);
			if (owned.sceneIndex == sceneIndex) {
				sceneEntities.push_back(entity);
			}
		}

		return sceneEntities;
	}

	std::vector<Entity> SceneManager::GetEntitiesInScene(const SceneAsset& scene) const {
		return GetEntitiesInScene(scene.sceneIndex);
	}

	const std::unordered_map<SceneGUID, SceneAsset>& SceneManager::GetAllSceneAssets() const {
		return scenes_;
	}

	bool SceneManager::CheckSceneNameExists(const std::string& sceneName) {
		std::cout << scenes_.size() << std::endl;
		for (const auto& [id, scene] : scenes_) {
			// Skip check if lengths differ
			if (scene.name.length() != sceneName.length()) continue;
			// Else, check if names are equal. Return true if equal, continue loop if not equal.
			bool isEqual = std::equal(scene.name.begin(), scene.name.end(), sceneName.begin(),
				[](char c1, char c2) {
					return std::tolower(static_cast<unsigned char>(c1)) ==
						std::tolower(static_cast<unsigned char>(c2));
				});

			if (isEqual) return true;
		}

		return false;
	}

	void SceneManager::UpdateScenePath(const SceneGUID& sceneIndex, const std::string& newPath) {
		for (auto& [id, scene] : scenes_) {
			if (scene.sceneIndex == sceneIndex) {
				scene.path = newPath;
			}
		}
	}


#pragma endregion

	// ---------------------------------------------------------------------------
	// DEBUG FUNCTIONS
	// ---------------------------------------------------------------------------
	void SceneManager::PrintAllScenesInfo() const {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		std::unordered_map<std::string, std::vector<Entity>> scenes;
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(false, SceneOwnedType)) {
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
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		if (!GetSceneAt(currentScene_, true).IsValid()) {
			Logger::Get().Info(DebugType::SceneManager) << "No active scene currently loaded." << std::endl;
			return;
		}

		Logger::Get().Info(DebugType::SceneManager) << "----- Current Scene Info -----" << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Loaded Scene: " << GetSceneAt(currentScene_).name << std::endl;
		Logger::Get().Info(DebugType::SceneManager) << "Loaded Entities Count: " << entityMgr.GetAllEntitiesWithComponent(false, LoadedTagType).size() << std::endl;

		std::ostream& logStream = Logger::Get().Info(DebugType::SceneManager);
		logStream << "Active Entities IDs: ";
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(false, LoadedTagType)) {
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
