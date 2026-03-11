/*****************************************************************//**
* \file   GameManager.h
*
* \primary_author
*		Jason Lou(80%)
* \secondary_author
*		Lur Pei Hao Stanley(20%)
*
* \brief	Manager responsible for managing game state.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#pragma once
#include "SceneManager.h"
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "ResourceManager.h"
#include "AudioSystem.h"
#include "WindowsSystem.h"
#include "SpriteRenderSystem.h"
#include "TextRenderSystem.h"
#include "LevelResetSystem.h"
#include "GuiSystem.h"
#include "Graphics.h"
#include "Core.h"
#include "AISystem.h"
#include "FileUtility.h"
#include "AnimationSystem.h"
#include "LayerManager.h"

#ifdef BUILD_EDITOR
#include "Editor.h"
#include <memory>
#endif

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>


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

	// Might rename this to a GameSystem later on.
	class GameManager : public ISystem {
	public:
		// Aliases
		using SceneFunction = void(*)(void);
	private:
		static GameManager* instance;
		std::string nextSceneToLoad_ = "";
		std::function<void()> deferredTask_ = nullptr;
		std::function<void()> overlayDrawCallback_ = nullptr;


		bool queuedLevelLoad_ = false;
		bool queuedLevelReset_ = false;
		bool queuedResume_ = false;
		std::string queuedSceneUnload_;
		std::string queuedLevelName_;

		struct SceneCallBacks {
			SceneFunction fpInitialize = nullptr;
			SceneFunction fpUpdate = nullptr;
			SceneFunction fpFree = nullptr;
		};
		std::unordered_map<SceneGUID, SceneCallBacks> sceneCallbacks_;
		std::unordered_set<SceneGUID> initializedScenes_;
		SceneGUID lastActiveScene_ = 0;

		struct RegisteredSystems {
			EntityStorage* ecs_ = nullptr;
			Graphics* gfx_ = nullptr;
			WindowsSystem* win_ = nullptr;
			InputSystem* input_ = nullptr;   // <-- MISSING MEMBER (add this)
			PhysicsSystem* physics_ = nullptr;
			ResourceManager* resourceManager_ = nullptr;
			AudioSystem* audio_ = nullptr;   // <-- store it
			SpriteRenderSystem* spriteRenderer_ = nullptr; // <-- store it if needed later
			TextRenderSystem* textRenderer_ = nullptr;
			AISystem* aiSystem_ = nullptr;
			GuiSystem* guiSystem_ = nullptr;
			CameraSystem* camera_ = nullptr;  
			LevelResetSystem* levelResetter_ = nullptr;
			TransformationSystem* transformation_ = nullptr;
			AnimationSystem* animationSystem_ = nullptr;			  
		};

		RegisteredSystems systems_;
		
#ifdef BUILD_EDITOR
		Editor::Editor* editor_ = nullptr; // ImGui Editor instance

		/* Play State Management */
		bool hasECSSnapshot = false;
		EntityStorage* editorECS_ = nullptr;
		std::unique_ptr<EntityStorage> runtimeECS_ = nullptr;
		Entity nextEntityIDSnapshot = 1u;
#endif

		float deltaTime = 0.0f;
		float globalTimeDilation = 1.0f; // 1.0f is normal speed, 0.5f is half speed, 2.0f is double speed

		GameManager() = default; // Private so no one can construct the GameManager, only the getInstance function will create the one and only one.
		~GameManager() = default;

		// Internal Checkers
		void CheckSystems() const;

		CoreEngine* engine_ = nullptr; // Pointer to the core engine for getting all system time
		//  Members for the system profiler display
		float profilerUpdateTimer_ = 0.0f;
		std::vector<SystemTime> profilerDisplayData_;

		// Internal Helpers
		void UpdateNativeScripts();

		// Preload all assets for a scene
		void PreloadSceneAssets(const std::string& sceneName);

		bool restartMenuInitialized = false;
		SceneGUID previousScene = 0;

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
		RegisteredSystems& GetSystems() { return systems_; }
		const RegisteredSystems& GetSystems() const { return systems_; }
		EntityStorage& GetECS() { return *systems_.ecs_; };
		const EntityStorage& GetECS() const { return *systems_.ecs_; }
		

		void SetEngine(CoreEngine& engine) { engine_ = &engine; } //  Setter for core Engine 

		/* GameManager Config */
		// Registers all the systems that the GameManager will use. Must be called before Initialize.
		void RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx,
			WindowsSystem& win, InputSystem& input, PhysicsSystem& physics,
			AudioSystem& audio, SpriteRenderSystem& renderer, TextRenderSystem& textRenderer,
			CameraSystem& camera, AISystem& ai, GuiSystem& gui, LevelResetSystem& levelResetter, TransformationSystem& transformation,
			AnimationSystem& animSys);
				
#ifdef BUILD_EDITOR
		void RegisterEditor(Editor::Editor& editor); // Register the ImGui Editor to the GameManager

		// Getter for the ImGui Editor instance
		Editor::Editor* GetEditor() const { return editor_; }

		/* Play State Management */
		void OnBeginPlay();
		void OnEndPlay();
#endif
		/* GameManager APIs */
		// Sets the function pointers for the current scene's lifecycle callbacks.
		void RegisterScriptToScene(SceneGUID sceneIndex, SceneFunction initFunction, SceneFunction updateFunction, SceneFunction freeFunction);
		void UnregisterScriptFromScene(SceneGUID sceneIndex);

		// Checks if game is currently in play mode or not
		bool IsGamePlaying() const;

		void SetOverlayCallback(std::function<void()> cb) { overlayDrawCallback_ = cb; }
		void SetNextScene(const std::string& sceneName) { nextSceneToLoad_ = sceneName; }
		std::string GetNextScene() const { return nextSceneToLoad_; }
		void InitiateLevelLoad(const std::string& levelName);

		// Helper to check if a scene is a level scene
		bool IsLevelScene(const std::string& name);

		// Queues a level load to be processed safely at the top of the next frame.
		// Use this instead of InitiateLevelLoad when calling from inside a scene
		// update callback (e.g. LevelSelection::Update) to avoid mutating the
		// loaded-scenes list while it is being iterated.
		void QueueLevelLoad(const std::string& levelName) {
			queuedLevelName_ = levelName;
			queuedLevelLoad_ = true;
		}

		// Getter for Previous scene
		SceneGUID GetPrevScene() const { return previousScene; }

		/* Game Cycle Functions */
		void Initialize() override;
		void Update() override;
		void Shutdown() override;
	};
}