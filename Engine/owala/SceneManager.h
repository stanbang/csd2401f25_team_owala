#pragma once
/*****************************************************************//**
 * \file	SceneManager.h
 * \brief	Manager responsible for managing scenes/levels
 *
 *
 * \author	Jason Lou
 * \date	11 September 2025
 *********************************************************************/
#include "ISystem.h"
#include "EntityStorage.h"
#include "SceneComponents.h"

namespace Framework {
	class SceneManager {
	private:
		static SceneManager* instance;
		std::string activeScene;
		EntityStorage* entityStorageRef_ = nullptr;

		SceneManager() = default; // Private so no one can construct the SceneManager, only the getInstance function will create the one and only one.
		~SceneManager() = default;

		// Internal Checkers
		void CheckStorage();
		void CheckStorage() const; // Const overload of the above checker
	public:
		// Singletons should not be cloneable or assignable.
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;

		// Gets current instance of the SceneManager. If it does not exist, creates a new instance.
		static SceneManager& GetInstance();
		// Check if an instance of InputManager exists during runtime, does not create a new instance if it does not exists. Returns true if it exists, false otherwise.
		static bool Exists();
		// Destroys the current instance of the SceneManager.
		static void DestroyInstance();

		/* SceneManager Config */
		// Sets the entity storage reference for the SceneManager to use.
		void SetEntityStorageRef(EntityStorage& entityStorage);

		/* Scene Management APIs */
		// Attaches the SceneOwned component to the entity, indicating which scene it belongs to.
		void AddEntityToScene(const std::string& scene, Entity entity);
		// Removes the SceneOwned component from the entity if attached.
		void RemoveEntityFromScene(Entity entity);
		// Sets or unsets the PersistentTag on the entity, indicating if it should persist across scene changes. Note that the entity must already have a SceneOwned component to be set as persistent.
		void SetEntityAsPersistent(Entity entity, bool shouldPersist = true);
		// Retrieves a list of all entities that belong to the specified scene, if the scene exists.
		std::vector<Entity> GetEntitiesInScene(const std::string& scene) const;
		// Loads the specified scene and marks all entities belonging to that scene as active for the Render and GameSystem. Also marks persistent entities.
		void LoadScene(const std::string& scene);
		// Unloads the specified scene and clears all active tags on entities.
		void UnloadScene(const std::string& scene); // Currently just clears all active tags. Future implementation may make use of the scene param to destroy entities belonging to that scene.
		// Check if the scene is currently loaded.
		bool IsSceneLoaded(const std::string& scene) const;
		// Gets the name of the currently active scene.
		std::string GetActiveScene() const { return activeScene; };

		/* DEBUG FUNCTIONS */
		void PrintAllScenesInfo() const;
		void PrintCurrentSceneInfo() const;
	};
}