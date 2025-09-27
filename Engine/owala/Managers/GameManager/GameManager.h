#pragma once
/*****************************************************************//**
 * \file	GameManager.h
 * \brief	Manager responsible for managing game state
 *
 *
 * \author	Jason Lou
 * \date	18 September 2025
 *********************************************************************/
#include "SceneManager.h"
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "WindowsSystem.h"
#include "SpriteRenderSystem.h"
#include "Graphics.h"

namespace Framework {
	enum class SystemType : unsigned char {
		RESOURCE_MANAGER = 0,
		ENTITY_STORAGE,
		GRAPHICS,
		WINDOWS_SYSTEM,
		INPUT_SYSTEM,
		PHYSICS_SYSTEM,
		AUDIO_SYSTEM
	};

	// Might rename this to a GameSystem later on. Either that or need to figure out how to decouple from the engine.
	class GameManager : public ISystem {
	public:
		// Aliases
		using SceneFunction = void(*)(void);
	private:
		static GameManager* instance;

		struct SceneCallBacks {
			SceneFunction fpInitialize = nullptr;
			SceneFunction fpUpdate = nullptr;
			SceneFunction fpFree = nullptr;
		};
		std::unordered_map<SceneGUID, SceneCallBacks> sceneCallbacks_;

		struct RegisteredSystems {
			EntityStorage* ecs_ = nullptr;
			Graphics* gfx_ = nullptr;
			WindowsSystem* win_ = nullptr;
			InputSystem* input_ = nullptr;   // <-- MISSING MEMBER (add this)
			PhysicsSystem* physics_ = nullptr;
			ResourceManager* resourceManager_ = nullptr;
			AudioSystem* audio_ = nullptr;   // <-- store it
			SpriteRenderSystem* spriteRenderer_ = nullptr; // <-- store it if needed later
		};

		RegisteredSystems systems_;

		float deltaTime = 0.0f;
		float globalTimeDilation = 1.0f; // 1.0f is normal speed, 0.5f is half speed, 2.0f is double speed

		GameManager() = default; // Private so no one can construct the GameManager, only the getInstance function will create the one and only one.
		~GameManager() = default;

		// Internal Checkers
		void CheckSystems() const;
	public:
		// GLOBAL STATES
		int testState = 0;

		// Singletons should not be cloneable or assignable.
		GameManager(const GameManager&) = delete;
		GameManager& operator=(const GameManager&) = delete;

		// Gets current instance of the GameManager. If it does not exist, creates a new instance.
		static GameManager& GetInstance();
		// Check if an instance of SceneManager exists during runtime, does not create a new instance if it does not exists. Returns true if it exists, false otherwise.
		static bool Exists();
		// Destroys the current instance of the SceneManager.
		static void DestroyInstance();

		/* Getters & Setters */
		// Retrieves the interval in seconds from the last frame to the current one (Read only).
		float GetDeltaTime() const { return deltaTime; }
		RegisteredSystems GetSystems() const { return systems_;  }

		/* GameManager Config */
		// Registers all the systems that the GameManager will use. Must be called before Initialize.
		void RegisterSystemsToUse(ResourceManager& rcm,	EntityStorage& ecs,	Graphics& gfx, WindowsSystem& win, InputSystem& input, PhysicsSystem& physics, AudioSystem& audio, SpriteRenderSystem& spriteRenderer);

		/* GameManager APIs */
		// Sets the function pointers for the current scene's lifecycle callbacks.
		void RegisterScriptToScene(SceneGUID sceneIndex, SceneFunction initFunction, SceneFunction updateFunction, SceneFunction freeFunction);
		void UnregisterScriptFromScene(SceneGUID sceneIndex);

		/* Game Cycle Functions */
		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override;
	};
}