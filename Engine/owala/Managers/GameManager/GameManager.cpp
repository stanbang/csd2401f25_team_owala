/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing game state
 *
 *
 * \author	Jason Lou
 * \date	18 September 2025
 *********************************************************************/
#include "GameManager.h"
#include "EntitySerializer.h"
#include <iostream>

// Scene Scripts
#include "SplashScreen.h"
#include "TestScene.h"

// ImGui headers
#ifdef BUILD_EDITOR
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif
//Core Engine headers for IMGUI
#include "Core.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <RestartMenu.h>

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
		if (!systems_.audio_) {
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
		EntityManager entityMgr(*systems_.ecs_);
		SceneManager& sceneMgr = SceneManager::GetInstance();
		for (Entity scriptableEntity : entityMgr.GetAllEntitiesWithComponents(NativeScriptsComponentType, ActiveTagType)) {
			if (entityMgr.GetComponent<SceneOwned>(scriptableEntity).sceneName != sceneMgr.GetActiveScene().name) continue;

			std::vector<NativeScriptsComponent::NativeScript>& nativeScripts = entityMgr.GetComponent<NativeScriptsComponent>(scriptableEntity).scripts;
			if (nativeScripts.empty()) continue;
			
			for (NativeScriptsComponent::NativeScript& script : nativeScripts) {
				if (!script.enabled) continue;

#ifdef BUILD_EDITOR
				if (!script.executeInEditMode && !editor_->IsGamePlaying()) continue;
#endif

				if (!script.Instance) {
					// Create and Bind Entity
					script.Instance = script.InstantiateScript();
					script.Instance->SetEntity(scriptableEntity);

					// Call Script's OnCreate
					script.Instance->OnCreate();
				}

				// Call Script's OnUpdate
				script.Instance->OnUpdate();
			}
		}
	}

	// ---------------------------------------------------------------------------
	// GameManager Config
	// ---------------------------------------------------------------------------
	void GameManager::RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx, WindowsSystem& win, InputSystem& input, PhysicsSystem& physics, AudioSystem& audio,
		SpriteRenderSystem& renderer,TextRenderSystem& textRenderer, CameraSystem& camera, AISystem& ai, GuiSystem& gui, LevelResetSystem& levelResetter,  TransformationSystem& transformation) {
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
		std::cout << "[GameManager] Successfully linked to all core systems" << std::endl;
		Logger::Get().Info(DebugType::GameManager) << "Successfully linked to all core systems" << std::endl;
	}
	#ifdef BUILD_EDITOR
	void GameManager::RegisterEditor(Editor::Editor& editor) {
		editor_ = &editor;
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

	void GameManager::Initialize() {
		CheckSystems();

		// Subscribe to Button Events for Scene Management and Game Flow
		MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {

			switch (event.action) {

			case ButtonAction::SWITCH_SCENE:
				if (!event.stringParam.empty()) {
					Logger::Get().Info(DebugType::GameManager)
						<< "Switching Scene to: " << event.stringParam << std::endl;

					// Use the SceneManager instance we already have access to
					SceneManager::GetInstance().LoadScene(event.stringParam);
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
		sceneMgr.LoadScene(sceneMgr.GetSceneAt(testScene));
	}

	void GameManager::Update() {
		// --- GAME UPDATE HERE ---

		SceneGUID currentSceneIndex = SceneManager::GetInstance().GetActiveScene().sceneIndex;

		// Detected Scene Change
		if (currentSceneIndex != lastInitializedScene_) {
			std::unordered_map<SceneGUID, SceneCallBacks>::iterator prevScene = sceneCallbacks_.find(lastInitializedScene_);
			if (prevScene != sceneCallbacks_.end() && prevScene->second.fpFree) {
				prevScene->second.fpFree();
			}

			std::unordered_map<SceneGUID, SceneCallBacks>::iterator newScene = sceneCallbacks_.find(currentSceneIndex);
			if (newScene != sceneCallbacks_.end() && newScene->second.fpInitialize) {
				newScene->second.fpInitialize();
			}

			lastInitializedScene_ = currentSceneIndex;
		}

		if (sceneCallbacks_.count(currentSceneIndex) && sceneCallbacks_[currentSceneIndex].fpUpdate) {
			sceneCallbacks_[currentSceneIndex].fpUpdate();

			UpdateNativeScripts();
		}

		//Render sprite in engine if not using editor
		#ifdef BUILD_ENGINE
		systems_.spriteRenderer_->Update();
		systems_.textRenderer_->Update();
		#endif
		//glfwSwapInterval(0); //just for testing to turn vsync off

		// --- END GAME UPDATE ---

		 //--- NEW: CREATE THE MAIN DOCKSPACE HOST WINDOW ---
		// This should be done here in the GameManager to provide a consistent
		 //editor environment across all scenes.
		//{
		//	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		//	const ImGuiViewport* viewport = ImGui::GetMainViewport();
		//	ImGui::SetNextWindowPos(viewport->WorkPos);
		//	ImGui::SetNextWindowSize(viewport->WorkSize);
		//	ImGui::SetNextWindowViewport(viewport->ID);
		//	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		//	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		//	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		//	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		//	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		//	// Begin the host window that will contain the dockspace
		//	ImGui::Begin("DockSpace Host", nullptr, window_flags);
		//	ImGui::PopStyleVar(3);

		//	// Submit the DockSpace itself
		//	ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		//	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

		//	ImGui::End(); // End the host window
		//}

		// --- RENDER RUNTIME STATS USING IMGUI ---
		/*ImGui::Begin("Game Info");
		ImGui::Text("--- Current Game Information ---");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();*/

		//ImGui::SetNextWindowSize(ImVec2(500, 250), ImGuiCond_FirstUseEver);
		RunRestartMenu();
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

	void GameManager::RunRestartMenu()
	{
		if (!restartMenuInitialized)
		{
			restartMenu_.Initialize(systems_.ecs_,systems_.levelResetter_,systems_.resourceManager_);
			restartMenuInitialized = true;
		}
		restartMenu_.Update();
	}

}