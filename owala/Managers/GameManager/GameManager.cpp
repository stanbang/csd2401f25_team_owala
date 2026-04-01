/*****************************************************************//**
 * \file	GameManager.cpp
 * \brief	Manager responsible for managing game state
 *
 *
 * \author	Jason Lou
 * \date	18 September 2025
 *********************************************************************/
#include "pch.h"
#include "GameManager.h"
#include "EntitySerializer.h"
#include "EntityManager.h"

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
		SpriteRenderSystem& renderer,TextRenderSystem& textRenderer, CameraSystem& camera, AISystem& ai, GuiSystem& gui, TransformationSystem& transformation,
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

		// Set active camera to first camera found
		if (systems_.camera_) {
			systems_.camera_->SetActiveCamera(0);
		}
	}

	void GameManager::OnEndPlay() {
		CheckSystems();
		if (!hasECSSnapshot) return;

		// Clear active camera
		if (systems_.camera_) {
			systems_.camera_->SetActiveCamera(0);
		}

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
		//layerManager_.LoadFromFile(FileIOKeys::LAYER_CONFIG_FILEPATH); // Load all configs directly for layer
		FileIOSystem::LoadWorldConfig();
		Logger::Get().Info(DebugType::GameManager)
			<< "Loaded LayerManager configuration" << std::endl;

		// Subscribe to Button Events for Scene Management and Game Flow
		MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {
			
			// If the button has a scene to unload on click, queue it for next frame.
			// Exception: SWITCH_SCENE_ADD handles its own unload via the queued additive swap
			// mechanism so that the unload and load happen atomically on the next frame.
			if (!event.sceneToUnload.empty() && event.action != ButtonAction::SWITCH_SCENE_ADD) {
				Logger::Get().Info(DebugType::GameManager)
					<< "Button queuing scene unload: " << event.sceneToUnload << std::endl;
				queuedSceneUnload_ = event.sceneToUnload;
			}

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

			case ButtonAction::SWITCH_SCENE_ADD:
				if (!event.stringParam.empty()) {
					Logger::Get().Info(DebugType::GameManager)
						<< "Additively Loading and Switching Scene to: " << event.stringParam << std::endl;

					SceneManager& sceneMgr = SceneManager::GetInstance();
					const SceneAsset& scene = sceneMgr.GetSceneByName(event.stringParam, true);
					if (!scene.IsValid()) {
						Logger::Get().Error(DebugType::GameManager)
							<< "SWITCH_SCENE_ADD failed: scene not found: " << event.stringParam << std::endl;
						break;
					}

					// Queue an instant additive swap for next frame to avoid
					// entity destruction issues mid-frame. No fade is applied
					// because additive overlay scenes (e.g. pause/options menus)
					// should appear and disappear instantly.
					queuedAdditiveSwap_ = true;
					queuedAdditiveLoad_ = event.stringParam;
					queuedAdditiveUnload_ = event.sceneToUnload;
				}
				else {
					Logger::Get().Error(DebugType::GameManager)
						<< "Button requested SWITCH_SCENE_ADD but stringParam (Scene Name) was empty!" << std::endl;
				}
				break;

			case ButtonAction::QUIT_GAME:
				Logger::Get().Info(DebugType::GameManager) << "Button requested QUIT_GAME" << std::endl;
				// Exit the application
				if (systems_.win_) {
					systems_.win_->setWindowShouldClose(true);
				}
				break;

			case ButtonAction::RESUME_GAME:
				Logger::Get().Info(DebugType::GameManager) << "Button requested RESUME_GAME" << std::endl;
				queuedResume_ = true;
				break;
			// Note: We do NOT handle PLAYSOUND here. AudioSystem handles that.
			default:
				break;
			}
			});

		SceneManager& sceneMgr = SceneManager::GetInstance();

		SceneGUID testScene{};
		// Create scene assets from file
		rapidjson::Document scenesData;
		if (FileIOSystem::LoadSceneAssets(scenesData)) {
			for (const rapidjson::Value& val : scenesData.GetArray()) {
				std::string sceneName = val[Serialization::SCENE_ASSET_NAME].GetString();
				sceneMgr.CreateSceneAsset(sceneName, val[Serialization::SCENE_ASSET_PATH].GetString());
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
		// Startup scene loading is now the responsibility of the Application (e.g. StageBoundApp).
		// Call gm.LoadStartupScene() after Configure() in your Application::Initialize().
	}

	void GameManager::Update() {
		// Tick the fade overlay every frame, even during scene transitions
		TickFadeOverlay(deltaTime > 0.0f ? deltaTime : Framework::Time::UnscaledDeltaTime());

		// Execute any deferred tasks from previous frame
		if (deferredTask_) {
			deferredTask_();
			deferredTask_ = nullptr;
		}

		// Process queued resume (deferred to avoid entity destruction mid frame).
		// Unloads all overlay scenes (configured in GameConfig::overlayScenes) and unpauses physics.
		if (queuedResume_) {
			queuedResume_ = false;
			for (const std::string& overlayScene : config_.overlayScenes) { // This is reading from Data/GameConfig.json to determine what scenes are considered overlays that should be unloaded on resume
				SceneManager::GetInstance().UnloadScene(overlayScene);
			}
			systems_.physics_->SetPaused(false);
		}

		// Process queued instant additive scene swap (e.g., PauseScene <-> OptionsMenu).
		// No fade is applied - overlay menus swap instantly.
		if (queuedAdditiveSwap_) {
			queuedAdditiveSwap_ = false;
			SceneManager& sm = SceneManager::GetInstance();
			// First unload the currently loaded additive scene if there is one, then load the new additive scene. This ensures that we don't have two additive scenes loaded at the same time, which could cause issues with entity management and callbacks.
			if (!queuedAdditiveUnload_.empty()) {
				Logger::Get().Info(DebugType::GameManager)
					<< "SWITCH_SCENE_ADD unloading scene: " << queuedAdditiveUnload_ << std::endl;
				sm.UnloadScene(queuedAdditiveUnload_);
				queuedAdditiveUnload_.clear();
			}
			if (!queuedAdditiveLoad_.empty()) {
				const SceneAsset& scene = sm.GetSceneByName(queuedAdditiveLoad_, true);
				if (scene.IsValid() && !scene.isLoaded) {
					sm.LoadScene(queuedAdditiveLoad_, SceneManagement::LoadSceneMode::Additive);
				}
				queuedAdditiveLoad_.clear();
			}
		}

		// Process any scene unload queued by a button click last frame.
		// If the unloaded scene is an overlay (configured in GameConfig::overlayScenes), unpause physics.
		if (!queuedSceneUnload_.empty()) {
			std::string sceneToUnload = std::move(queuedSceneUnload_);
			queuedSceneUnload_.clear();
			for (const std::string& overlayScene : config_.overlayScenes) {
				if (sceneToUnload == overlayScene) {
					systems_.physics_->SetPaused(false);
					break;
				}
			}
			SceneManager::GetInstance().UnloadScene(sceneToUnload);
		}

		// --- GAME UPDATE HERE ---
		if (queuedLevelLoad_) {
			std::string targetSceneName = queuedLevelName_;
			queuedLevelName_.clear();
			queuedLevelLoad_ = false;
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

		// Clear auto-config tracking for unloaded scenes
		for (auto it = autoConfiguredScenes_.begin(); it != autoConfiguredScenes_.end();) {
			if (loadedSet.find(*it) == loadedSet.end()) {
				it = autoConfiguredScenes_.erase(it);
			}
			else {
				++it;
			}
		}

		// Init Scenes that just became loaded
		for (SceneGUID sceneID : loadedScenes) {
			auto callback = sceneCallbacks_.find(sceneID);
			const bool hasLifecycle = (callback != sceneCallbacks_.end()) && (callback->second.fpInitialize || callback->second.fpFree);
			if (!hasLifecycle) continue;
			const bool firstTime = initializedScenes_.insert(sceneID).second;
			if (!firstTime) continue;
			if (callback->second.fpInitialize) {
				callback->second.fpInitialize();
			}
		}

		// Auto-detect Camera + Player entities and configure camera follow.
		// Runs for every newly loaded scene regardless of registered callbacks,
		// so any project gets camera follow for free when both entities exist.
		for (SceneGUID sceneID : loadedScenes) {
			if (!autoConfiguredScenes_.insert(sceneID).second) continue;

			EntityManager entityMgrAuto(*systems_.ecs_);

			std::vector<Entity> cameras = entityMgrAuto.GetAllEntitiesWithComponent(true, CameraType);
			if (cameras.empty()) continue;

			Entity mainCamera = cameras[0];
			systems_.camera_->SetActiveCamera(mainCamera);
			sceneMgr.SetEntityAsPersistent(mainCamera);
			Camera& gameCam = systems_.camera_->GetCamera(mainCamera);
			gameCam.isEditorControlled = false;

			std::vector<Entity> players = entityMgrAuto.GetAllEntitiesWithComponent(true, PlayerTagType);
			if (!players.empty()) {
				systems_.camera_->SetFollowTarget(mainCamera, players[0], 8.0f);
				systems_.camera_->CalculateAndSetBoundsFromPlatforms(mainCamera, 0.0f);
				const SceneAsset& sceneAsset = sceneMgr.GetSceneAt(sceneID);
				if (sceneAsset.name == "Level 4") {
					systems_.camera_->SetMapBounds(mainCamera, -1200.0f, 1300.0f, -1300.0f, 1350.0f);
				}
			}

			systems_.gfx_->setAlphaBlend(true);
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
				}
			}
			std::cout << "== End Persistent Entities == " << std::endl;
		}

		// Render sprites and text, then draw fade overlay on top
#ifndef BUILD_EDITOR
		systems_.textRenderer_->Update();
		systems_.spriteRenderer_->Update();
		if (overlayDrawCallback_) {
			overlayDrawCallback_();
		}
#else
	// In editor, sprites/text are rendered by the Core system loop.
	// Only draw the fade overlay during play mode.
		if (IsGamePlaying() && overlayDrawCallback_) {
			overlayDrawCallback_();
		}
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


		overlayDrawCallback_ = nullptr;
		fadeOnComplete_ = nullptr;

		instance = nullptr; // Reset instance pointer
	}

	// Helper to trigger the loading process
	// Fades to black first, then loads the loading scene. The loading scene
	// itself receives a FadeIn callback so it reveals itself automatically.
	void GameManager::InitiateLevelLoad(const std::string& targetSceneName) {
		std::string currentScene = SceneManager::GetInstance().GetActiveScene().name;
		// Only save the scene if it is an actual playable level
		if (IsLevelScene(currentScene)) {
			previousScene = SceneManager::GetInstance().GetActiveScene().sceneIndex;
		}

		systems_.physics_->SetPaused(false);

		// Reset global rotation in case we are transitioning from a scene that had it (e.g. BossScene)
		if (systems_.transformation_) {
			systems_.transformation_->ResetGlobalRotation();
		}

		SetNextScene(targetSceneName);
		PreloadSceneAssets(targetSceneName);

		// Fade to black, then swap to the loading scene while still black.
		// The loading scene's OnCreate will call FadeIn to reveal itself.
		FadeOut(0.5f, [this]() {
			SceneManager::GetInstance().LoadScene(config_.loadingScene);
		});
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

	void GameManager::Configure(const GameConfig& config) {
		config_ = config;
	}

	// Function to check if a scene is a level scene
	bool GameManager::IsLevelScene(const std::string& name) {
		for (const std::string& menuScene : config_.menuScenes) {
			if (name == menuScene) return false;
		}
		return true;
	}

	void GameManager::FadeOut(float duration, std::function<void()> onComplete) {
		fadeActive_ = true;
		fadeOut_ = true;
		fadeAlpha_ = 0.0f;
		fadeDuration_ = (duration > 0.0f) ? duration : 0.5f;
		fadeTimer_ = 0.0f;
		fadeOnComplete_ = onComplete;
	}

	void GameManager::FadeIn(float duration, std::function<void()> onComplete) {
		fadeActive_ = true;
		fadeOut_ = false;
		fadeAlpha_ = 1.0f;
		fadeDuration_ = (duration > 0.0f) ? duration : 0.5f;
		fadeTimer_ = 0.0f;
		fadeOnComplete_ = onComplete;
	}

	void GameManager::TickFadeOverlay(float dt) {
		if (!fadeActive_) return;
		fadeTimer_ += dt;
		const float t = std::min(fadeTimer_ / fadeDuration_, 1.0f);
		fadeAlpha_ = fadeOut_ ? t : (1.0f - t);

		if (t >= 1.0f) {
			fadeActive_ = false;
			// After a fade-in, ensure alpha is fully clear
			if (!fadeOut_) {
				fadeAlpha_ = 0.0f;
			}
			if (fadeOnComplete_) {
				auto cb = std::move(fadeOnComplete_);
				fadeOnComplete_ = nullptr;
				cb();
			}
		}
	}

	// Loads the startup scene defined in the GameConfig.
	// Must be called by the Application after Configure() during its own Initialize().
	void GameManager::LoadStartupScene() {
		if (!config_.startupScene.empty()) {
			SceneManager::GetInstance().LoadScene(config_.startupScene);
			Logger::Get().Info(DebugType::GameManager) << "Loaded startup scene: " << config_.startupScene << std::endl;
		}
		else {
			Logger::Get().Error(DebugType::GameManager) << "LoadStartupScene called but startupScene is not configured!" << std::endl;
		}
	}

}