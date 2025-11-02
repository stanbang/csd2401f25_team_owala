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

	// Hard Coded as if now. Will change to a more dynamic approach in the future when editor is in place.
	enum class SceneID : SceneGUID {
		SCENE_INVALID = 0,
		SCENE_SPLASH,
		SCENE_MENU,
		SCENE_LEVEL1,
		SCENE_LEVEL2,
		SCENE_LEVEL3,
		SCENE_LEVEL4
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

		EntityStorage* entityStorageRef_ = nullptr;
		std::unordered_map<SceneGUID, SceneAsset> scenes_; // Storage for all created scenes, mapped by their unique GUID.

		SceneManager(); // Private so no one can construct the SceneManager, only the getInstance function will create the one and only one.
		~SceneManager() = default;

		// Internal Checkers
		void CheckStorage();
		void CheckStorage() const; // Const overload of the above checker

		// Internal Helpers
		SceneAsset* GetSceneAssetMutable(SceneGUID sceneIndex);
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
		void SetEntityStorageRef(EntityStorage& entityStorage);

		/* Scene Management APIs */
		// Create an empty new Scene at runtime with the given name. Returns the GUID of the created scene if successful.
		SceneGUID CreateSceneAsset(const std::string& sceneName, const std::string& path = {}, const bool entitiesPopulated = false);
		// Gets the scene by name. Returns an SceneAsset Object. If the scene does not exist, returns an invalid SceneAsset object.
		const SceneAsset& GetSceneByName(const std::string& sceneName, bool silenceError = false) const;
		// Gets the scene by ID. Returns an SceneAsset Object. If the scene does not exist, returns an invalid SceneAsset object.
		const SceneAsset& GetSceneAt(const SceneGUID sceneIndex, bool silenceError = false) const;
		// Gets the currently active scene. If no scene is active, returns an invalid SceneAsset object.
		const SceneAsset& GetActiveScene() const;
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		void LoadScene(const std::string& sceneName);
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		// Overloaded Function: Uses the ID of the scene instead.
		void LoadScene(const SceneGUID sceneIndex);
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		// Overlaoded Function: Uses the Scene Object instead.
		void LoadScene(const SceneAsset& scene);
		// Unloads the current scene.
		void UnloadScene(); // Currently just clears all active tags. Future implementation may make use of the scene param to destroy entities belonging to that scene.
		// Attaches the SceneOwned component to the entity, indicating which scene it belongs to. Uses name of scene in string.
		void AddEntityToScene(const std::string& sceneName, Entity entity);
		// Attach the SceneOwned component to the entity, indicating which scene it belongs to. 
		// Overloaded Function: Uses the ID of the scene instead.
		void AddEntityToScene(const SceneGUID sceneIndex, Entity entity);
		// Attach the SceneOwned component to the entity, indicating which scene it belongs to. 
		// Overloaded Function: Uses the Scene Object instead.
		void AddEntityToScene(const SceneAsset& scene, Entity entity);
		// Sets or unsets the PersistentTag on the entity, indicating if it should persist across scene changes. Note that the entity must already have a SceneOwned component to be set as persistent.
		void SetEntityAsPersistent(Entity entity, bool shouldPersist = true);
		// Retrieves a list of all entities that belong to the specified scene, if the scene exists.
		std::vector<Entity> GetEntitiesInScene(const std::string& scene) const;
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