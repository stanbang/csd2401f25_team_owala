/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing game state
 *
 *
 * \author	Jason Lou
 * \date	18 September 2025
 *********************************************************************/
#include "pch.h"
#include "GameManager.h"
#include "EntitySerializer.h"

// Scene Scripts
#include "SplashScreen.h"
#include "TestScene.h"
#include "CutsceneIntro.h"
#include "HowToPlay.h"
#include "LoadingScreenController.h"
#include "TransitionController.h"

//Core Engine headers for IMGUI
#include "Core.h"

namespace Framework {
	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	GameManager* GameManager::instance = nullptr; // Initialize the static instance pointer to nullptr
	
	GameManager& GameManager::GetInstance() {
		if (!instance) {
			instance = new GameManager();
			Logger::Get().Info(DebugType::GameManager) << "GameManager Instantiated!" << std::endl;
		}
		return *instance;
	}

	bool GameManager::Exists() {
		return instance != nullptr;
	}

	void GameManager::DestroyInstance() {
		delete instance;
		instance = nullptr;
	}

	// ---------------------------------------------------------------------------
	// GameManager Internals
	// ---------------------------------------------------------------------------
	void GameManager::CheckSystems() const {
		if (!systems_.resourceManager_) {
			throw GameException(DebugType::GameManager,"ResourceManager not registered!");
		}
		if (!systems_.ecs_) {
			throw GameException(DebugType::GameManager, "EntityStorage not registered!");
		}
		if (!systems_.gfx_) {
			throw GameException(DebugType::GameManager, "Graphics not registered!");
		}
		if (!systems_.win_) {
			throw GameException(DebugType::GameManager, "WindowsSystem not registered!");
		}
		if (!systems_.input_) {
			throw GameException(DebugType::GameManager, "InputSystem not registered!");
		}
		if (!systems_.physics_) {
			throw GameException(DebugType::GameManager, "PhysicsSystem not registered!");
		}
		if (!systems_.audio_) {
			throw GameException(DebugType::GameManager, "AudioSystem not registered!");
		}
		if (!systems_.spriteRenderer_) {
			throw GameException(DebugType::GameManager, "SpriteRenderSystem not registered!");
		}		
		if (!systems_.textRenderer_) {
			throw GameException(DebugType::GameManager, "TextRenderSystem not registered!");
		}
		if (!systems_.camera_) {
			throw GameException(DebugType::GameManager, "CameraSystem not registered!");
		}
		if (!systems_.levelResetter_) {
			throw GameException(DebugType::GameManager, "LevelResetSystem not registered!");
		}
		if (!SceneManager::Exists()) {
			throw GameException(DebugType::GameManager, "SceneManager not registered!");
		}

		if (!systems_.aiSystem_) {
			throw GameException(DebugType::GameManager, "AI system not registered!");
		}
	}

	void GameManager::UpdateNativeScripts() {
		static int count = 0;
		EntityManager entityMgr(*systems_.ecs_);
		//SceneManager& sceneMgr = SceneManager::GetInstance();
		
		// Get all entities with NativeScriptsComponent and ActiveTag to log them
		std::vector<Entity> scriptEntities = entityMgr.GetAllEntitiesWithComponents(true, NativeScriptsComponentType, LoadedTagType);
		
		for (Entity scriptableEntity : entityMgr.GetAllEntitiesWithComponents(true, NativeScriptsComponentType, LoadedTagType)) {
			if (!entityMgr.ValidateEntity(scriptableEntity)) continue;
			std::vector<NativeScriptsComponent::NativeScript>& nativeScripts = entityMgr.GetComponent<NativeScriptsComponent>(scriptableEntity).scripts;

			if (nativeScripts.empty()) continue;
			
			for (NativeScriptsComponent::NativeScript& script : nativeScripts) {
				if (!script.enabled) continue;

#ifdef BUILD_EDITOR
				if (!script.executeInEditMode && !IsGamePlaying()) continue;
#endif

				if (!script.Instance && script.InstantiateScript) {
					// Create and Bind Entity
					script.Instance = script.InstantiateScript();
					script.Instance->SetEntity(scriptableEntity);

					// Call Script's OnCreate
					script.Instance->OnCreate();
				}

				// Call Script's OnUpdate
				if (script.Instance) {
					script.Instance->OnUpdate();
				}
			}
		}
	}

	// ---------------------------------------------------------------------------
	// GameManager Config
	// ---------------------------------------------------------------------------
	void GameManager::RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx, WindowsSystem& win, InputSystem& input, PhysicsSystem& physics, AudioSystem& audio,
		SpriteRenderSystem& renderer,TextRenderSystem& textRenderer, CameraSystem& camera, AISystem& ai, GuiSystem& gui, LevelResetSystem& levelResetter,  TransformationSystem& transformation,
		AnimationSystem& animSys) {
		systems_.resourceManager_ = &rcm;
		systems_.ecs_ = &ecs;
		systems_.gfx_ = &gfx;
		systems_.win_ = &win;
		systems_.input_ = &input;
		systems_.physics_ = &physics;
		systems_.audio_ = &audio;
		systems_.spriteRenderer_ = &renderer;
		systems_.textRenderer_ = &textRenderer;
		systems_.camera_ = &camera; 
		systems_.aiSystem_ = &ai;
		systems_.guiSystem_ = &gui;
		systems_.levelResetter_ = &levelResetter;
		systems_.transformation_ = &transformation;
		systems_.animationSystem_ = &animSys;
		std::cout << "[GameManager] Successfully linked to all core systems" << std::endl;
		Logger::Get().Info(DebugType::GameManager) << "Successfully linked to all core systems" << std::endl;
	}

#ifdef BUILD_EDITOR
	// ---------------------------------------------------------------------------
	// Play State Management
	// ---------------------------------------------------------------------------
	void GameManager::RegisterEditor(Editor::Editor& editor) {
		editor_ = &editor;
	}

	void GameManager::OnBeginPlay() {
		CheckSystems();
		if (hasECSSnapshot) return;

		// Snapshot editor world.
		editorECS_ = systems_.ecs_;
		nextEntityIDSnapshot = nextEntityID;

		// Create runtime world.
		runtimeECS_ = std::make_unique<EntityStorage>(*editorECS_);

		systems_.ecs_ = runtimeECS_.get();

		hasECSSnapshot = true;

		systems_.physics_->SetPaused(false);
		SceneManager::GetInstance().OnEnterPlay();
		initializedScenes_.clear();
		lastActiveScene_ = 0;
	}

	void GameManager::OnEndPlay() {
		CheckSystems();
		if (!hasECSSnapshot) return;

		systems_.ecs_ = editorECS_;
		nextEntityID = nextEntityIDSnapshot;

		systems_.physics_->SetPaused(true);
		systems_.audio_->stopAll();
		SceneManager::GetInstance().OnExitPlay();

		// Cleaning
		runtimeECS_.reset();
		editorECS_ = nullptr;
		hasECSSnapshot = false;
		initializedScenes_.clear();
		lastActiveScene_ = 0;
	}
#endif


	// ---------------------------------------------------------------------------
	// GameManager APIs
	// ---------------------------------------------------------------------------
	void GameManager::RegisterScriptToScene(SceneGUID sceneIndex, SceneFunction initFunction, SceneFunction updateFunction, SceneFunction freeFunction) {
		sceneCallbacks_[sceneIndex] = SceneCallBacks{ initFunction, updateFunction, freeFunction };
	}

	void GameManager::UnregisterScriptFromScene(SceneGUID sceneIndex) {
		sceneCallbacks_.erase(sceneIndex);
	}

	bool GameManager::IsGamePlaying() const {
#ifdef BUILD_EDITOR
		return (editor_ != nullptr) ? editor_->IsGamePlaying() : true;
#else
		return true;
#endif
	}

	void GameManager::Initialize() {
		CheckSystems();
		TransitionController::Reset();
		// Subscribe to Button Events for Scene Management and Game Flow
		MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {

			switch (event.action) {

			case ButtonAction::SWITCH_SCENE:
				if (!event.stringParam.empty()) {
					Logger::Get().Info(DebugType::GameManager)
						<< "Switching Scene to: " << event.stringParam << std::endl;

					// Use the SceneManager instance we already have access to
					//SceneManager::GetInstance().LoadScene(event.stringParam);

					// Use GameManager to initiate async level load with loading screen
					queuedLevelName_ = event.stringParam;
					queuedLevelLoad_ = true;
				}
				else {
					Logger::Get().Error(DebugType::GameManager)
						<< "Button requested SWITCH_SCENE but stringParam (Scene Name) was empty!" << std::endl;
				}
				break;

			case ButtonAction::QUIT_GAME:
				Logger::Get().Info(DebugType::GameManager) << "Button requested QUIT_GAME" << std::endl;
				// Exit the application
				if (systems_.win_) {
					systems_.win_->setWindowShouldClose(true);
				}
				break;

				// Note: We do NOT handle PLAYSOUND here. AudioSystem handles that.
			default:
				break;
			}
			});

		MessageBus::Instance().Subscribe<PlayerDeathEvent>([this](const PlayerDeathEvent& event) {
			// Change to RestartMenu when death animation finishes
			if (event.deathAnimationFinished) {
				// If transitioning to another scene already, do nothing
				if (!GetNextScene().empty()) return;

				// Only change to RestartMenu if not already there
				if (SceneManager::GetInstance().GetActiveScene().name != "RestartMenu") {
					Logger::Get().Info(DebugType::GameManager) << "Changing to RestartMenu scene" << std::endl;
					//GetSystems().levelResetter_->RunReset(); // Don't reset the level before the curtains close, do it after inside InitiateLevelLoad
					//SceneManager::GetInstance().LoadScene("RestartMenu"); //Old code without transitions
					this->InitiateLevelLoad("RestartMenu"); // Correct way to load with transitions
				}
			}
		});

		SceneManager& sceneMgr = SceneManager::GetInstance();

		SceneGUID testScene{};
		// Create scene assets from file
		rapidjson::Document scenesData;
		if (FileIOSystem::LoadSceneAssets(scenesData)) {
			for (const rapidjson::Value& val : scenesData.GetArray()) {
				std::string sceneName = val[Serialization::SCENE_ASSET_NAME].GetString();
				SceneGUID sceneID = sceneMgr.CreateSceneAsset(sceneName, val[Serialization::SCENE_ASSET_PATH].GetString());

				if (sceneName == "Splash Screen") {
					RegisterScriptToScene(sceneID, SplashScreen::Initialize, SplashScreen::Update, SplashScreen::Free);
					continue;
				}
				if (sceneName == "Test Scene") {
					RegisterScriptToScene(sceneID, TestScene::Initialize, TestScene::Update, TestScene::Free);
					testScene = sceneID;
					continue;
				}
				if (sceneName == "Cutscene Intro") {
					RegisterScriptToScene(sceneID, CutsceneIntro::Initialize, CutsceneIntro::Update, CutsceneIntro::Free);
					continue;
				}
				if (sceneName == "HowToPlay") {
					RegisterScriptToScene(sceneID, HowToPlay::Initialize, HowToPlay::Update, HowToPlay::Free);
					continue;
				}
				// all other scenes use TestScene.cpp for now
				RegisterScriptToScene(sceneID, TestScene::Initialize, TestScene::Update, TestScene::Free);
			}
		}
		else {
			Logger::Get().Error(DebugType::GameManager) << "Failed to load scene assets from file" << std::endl;
		}

		// Testing Scene Creation - Hard Coded at the moment
		//SceneGUID splashScreen = sceneMgr.CreateSceneAsset("Splash Screen");
		//testScene = sceneMgr.CreateSceneAsset("Test Scene");

		//RegisterScriptToScene(splashScreen, SplashScreen::Initialize, SplashScreen::Update, SplashScreen::Free);
		//RegisterScriptToScene(testScene, TestScene::Initialize, TestScene::Update, TestScene::Free);

		Logger::Get().Info(DebugType::GameManager) << "Default Scene: " << sceneMgr.GetSceneAt(testScene).name << ", GUID: " << testScene << std::endl;
		//sceneMgr.LoadScene(sceneMgr.GetSceneAt(testScene));
		sceneMgr.LoadScene("MainMenu");
	}

	void GameManager::Update() {
		// Execute any deferred tasks from previous frame
		if (deferredTask_) {
			deferredTask_();
			deferredTask_ = nullptr;
		}

		// --- GAME UPDATE HERE ---
		if (queuedLevelLoad_) {
			std::string targetSceneName = queuedLevelName_;
			queuedLevelName_.clear();
			queuedLevelLoad_ = false;

			if (queuedLevelReset_) {
				queuedLevelReset_ = false;
				if (systems_.levelResetter_) {
					systems_.levelResetter_->RunReset();
				}
			}

			InitiateLevelLoad(targetSceneName);
		}

		SceneManager& sceneMgr = SceneManager::GetInstance();
		const std::vector<SceneGUID>& loadedScenes = sceneMgr.GetLoadedScenes();
		std::unordered_set<SceneGUID> loadedSet;
		for (SceneGUID loadedSceneID : loadedScenes) {
			loadedSet.insert(loadedSceneID);
		}

		// Free Scenes that are no longer loaded
		for (auto it = initializedScenes_.begin(); it != initializedScenes_.end();) {
			const SceneGUID sceneID = *it;

			if (loadedSet.find(sceneID) == loadedSet.end()) {
				std::unordered_map<SceneGUID, SceneCallBacks>::iterator callback = sceneCallbacks_.find(sceneID);
				if (callback != sceneCallbacks_.end() && callback->second.fpFree) {
					callback->second.fpFree();
				}

				it = initializedScenes_.erase(it);
			}
			else {
				++it;
			}
		}

		// Init Scenes that just became loaded
		for (SceneGUID sceneID : loadedScenes) {
			auto callback = sceneCallbacks_.find(sceneID);
			const bool hasLifecycle = (callback != sceneCallbacks_.end()) && (callback->second.fpInitialize || callback->second.fpFree);

			if (!hasLifecycle) {
				continue;
			}

			const bool firstTime = initializedScenes_.insert(sceneID).second;
			if (!firstTime) continue;

			if (callback->second.fpInitialize) {
				callback->second.fpInitialize();
			}
		}

#ifdef BUILD_EDITOR
		// Clear History when changing scenes
		const SceneGUID activeScene = sceneMgr.GetActiveScene().sceneIndex;
		if (activeScene != lastActiveScene_) {
			editor_->GetHistoryMgr()->ClearHistory();
			lastActiveScene_ = activeScene;
		}
#endif

		// Update all loaded scenes
		for (SceneGUID loadedSceneID : loadedScenes) {
			auto callback = sceneCallbacks_.find(loadedSceneID);
			if (callback != sceneCallbacks_.end() && callback->second.fpUpdate) {
				callback->second.fpUpdate();
			}
		}

		// Always update native scripts
		UpdateNativeScripts();

		if (systems_.input_->KeyPressed(InputSystem::Key::F2)) {
			EntityManager entityMgr(GetECS());
			std::vector<Entity> persistentEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
			std::cout << persistentEntities.size() << std::endl;
			std::cout << "Persistent Entities: " << std::endl;
			for (Entity entity : persistentEntities) {
				if (entityMgr.ValidateEntity(entity)) {
					std::cout << "Entity ID: " << entity << " | Entity Name: " << entityMgr.GetProperties(entity).name << std::endl;
;				}
			}
			std::cout << "== End Persistent Entities == " << std::endl;
		}

		//Render sprite in engine if not using editor
		#ifdef BUILD_ENGINE
		systems_.textRenderer_->Update();
		systems_.spriteRenderer_->Update();
		#endif

		// --- END GAME UPDATE ---
	}

	void GameManager::Shutdown() {
		// Call the free function for the current scene before shutting down.
		SceneGUID currentSceneIndex = SceneManager::GetInstance().GetActiveScene().sceneIndex;
		if (sceneCallbacks_.count(currentSceneIndex) && sceneCallbacks_[currentSceneIndex].fpFree) {
			sceneCallbacks_[currentSceneIndex].fpFree();
		}

		// Free all ScriptableEntities
		EntityManager entityMgr(*systems_.ecs_);

		// Renderer Cleanup
		systems_.spriteRenderer_->Shutdown();
		delete systems_.spriteRenderer_;
		systems_.spriteRenderer_ = nullptr;

		systems_.textRenderer_->Shutdown();
		delete systems_.textRenderer_;
		systems_.textRenderer_ = nullptr;

		// Level resetter cleanup
		delete systems_.levelResetter_;

		instance = nullptr; // Reset instance pointer
	}

	// Helper to trigger the loading process
	void GameManager::InitiateLevelLoad(const std::string& targetSceneName) {

		std::string currentSceneName = SceneManager::GetInstance().GetActiveScene().name;
		bool currentIsLevel = IsLevelScene(currentSceneName);
		bool targetIsLevel = IsLevelScene(targetSceneName);

		// Level > Level (Direct Transition, No Loading Screen)
		if (currentIsLevel && targetIsLevel) {
			Logger::Get().Info(DebugType::GameManager) << "Level->Level Transition: " << targetSceneName << std::endl;

			// Close Curtains
			TransitionController::PlayAllClose([this, targetSceneName]() {
				deferredTask_ = [targetSceneName]() {
					SceneManager::GetInstance().LoadScene(targetSceneName);
					TransitionController::s_startNextSceneClosed = true;
					};
				});
		}
		// Menu > Level (Show Loading Screen)
		else if (!currentIsLevel && targetIsLevel) {
			Logger::Get().Info(DebugType::GameManager) << "Menu->Level Transition via Loading Screen" << std::endl;
			SetNextScene(targetSceneName); // Store target for LoadingScreenController
			PreloadSceneAssets(targetSceneName);

			// Just load loading screen immediately
			SceneManager::GetInstance().LoadScene("LoadingScreen");
		}

		// Level > Menu (Close Curtains, then Loading Screen, then Menu)
		else if (currentIsLevel && !targetIsLevel) {
			Logger::Get().Info(DebugType::GameManager) << "Level->Menu Transition" << std::endl;
			SetNextScene(targetSceneName); // Store target (e.g., MainMenu)

			// Close Curtains
			TransitionController::PlayAllClose([this]() {
				// Reset Level
				this->GetSystems().levelResetter_->RunReset();

				// Load Loading Screen
				deferredTask_ = []() {
					SceneManager::GetInstance().LoadScene("LoadingScreen");
					};
				});
		}

		// Menu -> Menu (Direct, no curtains)
		else {
			SceneManager::GetInstance().LoadScene(targetSceneName);
		}
	}

	void GameManager::PreloadSceneAssets(const std::string& sceneName) {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		ResourceManager& rcm = *systems_.resourceManager_;

		// Find the scene file path
		const SceneAsset& scene = sceneMgr.GetSceneByName(sceneName);
		if (!scene.IsValid()) {
			Logger::Get().Error(DebugType::GameManager) << "Cannot preload assets: Scene not found: " << sceneName << std::endl;
			return;
		}

		std::string jsonPath = scene.path;
		Logger::Get().Info(DebugType::GameManager) << "Parsing scene JSON for assets: " << jsonPath << std::endl;

		// Open JSON file
		std::ifstream ifs(jsonPath);
		if (!ifs.is_open()) {
			Logger::Get().Error(DebugType::GameManager) << "Failed to open scene file: " << jsonPath << std::endl;
			return;
		}

		rapidjson::IStreamWrapper isw(ifs);
		rapidjson::Document doc;
		doc.ParseStream(isw);

		if (!doc.IsObject() || !doc.HasMember("entities")) return;

		// Loop through entities and find textures
		const rapidjson::Value& entities = doc["entities"];
		for (const auto& entity : entities.GetArray()) {
			if (!entity.HasMember("components")) continue;
			const auto& components = entity["components"];

			// Check TextureComponent
			if (components.HasMember("TextureComponent")) {
				const auto& texComp = components["TextureComponent"];
				if (texComp.HasMember("texture_key")) {
					std::string key = texComp["texture_key"].GetString();
					std::string path = rcm.GetAssetPath(key);

					if (!path.empty() && !rcm.has<Texture>(key)) {
						rcm.LoadTextureAsync(path);
					}
				}
			}

			// Check ParticleEmitterComponent
			if (components.HasMember("ParticleEmitterComponent")) {
				const auto& partComp = components["ParticleEmitterComponent"];
				if (partComp.HasMember("particle_texture_key")) {
					std::string key = partComp["particle_texture_key"].GetString();
					std::string path = rcm.GetAssetPath(key);

					if (!path.empty() && !rcm.has<Texture>(key)) {
						rcm.LoadTextureAsync(path);
					}
				}
			}
		}
	}

	// Function to check if a scene is a level scene
	bool GameManager::IsLevelScene(const std::string& name) {
		if (name == "MainMenu" || 
			name == "LevelSelect" || 
			name == "SplashScreen" || 
			name == "RestartMenu" || 
			name == "LoadingScreen" || 
			name == "HowToPlay" ||
			name == "OptionsMenu" ||
			name == "Cutscene Intro")
			return false;
		return true;
	}

}