/*****************************************************************//**
* \file   SceneManager.h
*
* \primary_author
*      Jason Lou
* \secondary_author
*
*
* \brief	Manager responsible for managing scenes/levels, including 
*			loading and unloading, management of entity-level associations.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "EntityStorage.h"
#include "SceneComponents.h"

namespace SceneManagement {
	using SceneGUID = unsigned int;

	enum class LoadSceneMode : unsigned char {
		Single = 0,
		Additive
	};

	struct SceneAsset {
		SceneGUID sceneIndex = 0; // Unique identifier for the scene.
		std::string name; // Name of the scene.
		std::string path; // Path to the scene asset file.
		bool isLoaded = false;
		bool isPopulated = false; // Indicate if it has been populated with asset file data
		bool IsValid() const { return sceneIndex != 0; };
	};
}

namespace Framework {
	using namespace SceneManagement;

	class SceneManager {
	private:
		static SceneManager* instance;
		static SceneGUID nextGUID; // Static counter to assign unique GUIDs to new scenes.
		SceneAsset invalidScene; // Predefined invalid scene object for error handling.

		SceneGUID prevScene_ = 0; // Index of the previous scene
		SceneGUID currentScene_ = 0; // Index of the current scene
		SceneGUID nextScene_ = 0; // Index of the next scene to be loaded

		std::unordered_map<SceneGUID, SceneAsset> scenes_; // Storage for all created scenes, mapped by their unique GUID.

		std::vector<SceneGUID> loadedScenes_;

		/* Predefined Scenes */
		static constexpr SceneGUID INVALID_SCENEID = 0;
		static constexpr const char* INVALID_SCENENAME = "InvalidScene";
		static constexpr SceneGUID DDOL_SCENEID = 0xFFFFFFFFu;
		static constexpr const char* DDOL_SCENENAME = "DontDestroyOnLoad";

		SceneManager(); // Private so no one can construct the SceneManager, only the getInstance function will create the one and only one.
		~SceneManager() = default;

		// Internal Checkers
		//void CheckStorage();
		//void CheckStorage() const; // Const overload of the above checker

		// Internal Helpers
		SceneAsset* GetSceneAssetMutable(SceneGUID sceneIndex);
		void EnsureScenePopulated(SceneGUID sceneIndex);
		void RebuildActiveTags();

#ifdef BUILD_EDITOR
		/* Scene Context Handling */
	private:
		struct SceneContext {
			std::unordered_map<SceneGUID, SceneAsset> scenes;
			std::vector<SceneGUID> loadedScenes;
			SceneGUID activeScene = 0;
		};

		bool isPlayMode_ = false;
		bool contextCaptured = false;
		SceneContext prePlayContext;
	public:
		void OnEnterPlay();
		void OnExitPlay();
		/* Scene Context Handling */
#endif
	public:
		// Singletons should not be cloneable or assignable.
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		// Gets current instance of the SceneManager. If it does not exist, creates a new instance.
		static SceneManager& GetInstance();
		// Check if an instance of SceneManager exists during runtime, does not create a new instance if it does not exists. Returns true if it exists, false otherwise.
		static bool Exists();
		// Destroys the current instance of the SceneManager.
		static void DestroyInstance();

		/* SceneManager Config */
		// Sets the entity storage reference for the SceneManager to use.
		//void SetEntityStorageRef(EntityStorage& entityStorage);

		/* Scene Management APIs */
		// Create an empty new Scene at runtime with the given name. Returns the GUID of the created scene if successful.
		SceneGUID CreateSceneAsset(const std::string& sceneName, const std::string& path = {}, const bool entitiesPopulated = false);
		// Gets the scene by name. Returns an SceneAsset Object. If the scene does not exist, returns an invalid SceneAsset object.
		const SceneAsset& GetSceneByName(const std::string& sceneName, bool silenceError = false) const;
		// Gets the scene by ID. Returns an SceneAsset Object. If the scene does not exist, returns an invalid SceneAsset object.
		const SceneAsset& GetSceneAt(const SceneGUID sceneIndex, bool silenceError = false) const;
		// Gets the currently active scene. If no scene is active, returns an invalid SceneAsset object.
		const SceneAsset& GetActiveScene() const;
		bool SetActiveScene(const SceneAsset& scene);
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		void LoadScene(const std::string& sceneName, LoadSceneMode mode = LoadSceneMode::Single);
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		// Overloaded Function: Uses the ID of the scene instead.
		void LoadScene(SceneGUID sceneIndex, LoadSceneMode mode = LoadSceneMode::Single);
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		// Overloaded Function: Uses the Scene Object instead.
		void LoadScene(const SceneAsset& scene, LoadSceneMode mode = LoadSceneMode::Single);
		// Destroys all entities associated with the given Scene and removes the Scene from the SceneManager's loaded scenes.
		void UnloadScene(const std::string& sceneName);
		// Destroys all entities associated with the given Scene and removes the Scene from the SceneManager's loaded scenes.
		// Overloaded Function: Uses the ID of the scene instead.
		void UnloadScene(SceneGUID sceneIndex);
		// Destroys all entities associated with the given Scene and removes the Scene from the SceneManager's loaded scenes.
		// Overloaded Function: Uses the Scene Object instead.
		void UnloadScene(const SceneAsset& scene);
		// Unloads the current scene.
		void AddEntityToScene(const std::string& sceneName, Entity entity);
		// Attach the SceneOwned component to the entity, indicating which scene it belongs to. 
		// Overloaded Function: Uses the ID of the scene instead.
		void AddEntityToScene(SceneGUID sceneIndex, Entity entity);
		// Attach the SceneOwned component to the entity, indicating which scene it belongs to. 
		// Overloaded Function: Uses the Scene Object instead.
		void AddEntityToScene(const SceneAsset& scene, Entity entity);
		// Sets or unsets the PersistentTag on the entity, indicating if it should persist across scene changes. Note that the entity must already have a SceneOwned component to be set as persistent.
		void SetEntityAsPersistent(Entity entity, bool shouldPersist = true);
		// Retrieves a list of all entities that belong to the specified scene, if the scene exists.
		std::vector<Entity> GetEntitiesInScene(const std::string& sceneName) const;
		// Retrieves a list of all entities that belong to the specified scene, if the scene exists.
		std::vector<Entity> GetEntitiesInScene(SceneGUID sceneIndex) const;
		// Retrieves a list of all entities that belong to the specified scene, if the scene exists.
		std::vector<Entity> GetEntitiesInScene(const SceneAsset& scene) const;
		// scenes_ map getter
		const std::unordered_map<SceneGUID, SceneAsset>& GetAllSceneAssets() const;

		// Checker for duplicate scene names. Return true if scene name exists, false otherwise.
		bool CheckSceneNameExists(const std::string& sceneName);
		// Update scene asset
		void UpdateScenePath(const SceneGUID& sceneIndex, const std::string& newPath);

		/* DEBUG FUNCTIONS */
		void PrintAllScenesInfo() const;
		void PrintCurrentSceneInfo() const;
	};
}