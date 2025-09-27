/*****************************************************************//**
 * \file	SceneSystem.cpp
 * \brief	Manager responsible for managing game state
 *
 *
 * \author	Jason Lou
 * \date	18 September 2025
 *********************************************************************/
#include "GameManager.h"
#include <iostream>

// Scene Scripts
#include "SplashScreen.h"
#include "TestScene.h"

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
			throw std::runtime_error("[GameManager] SpriteRenderSystem not registered!");
		}
		if (!SceneManager::Exists()) {
			throw GameException(DebugType::GameManager, "SceneManager not registered!");
		}
	}

	// ---------------------------------------------------------------------------
	// GameManager Config
	// ---------------------------------------------------------------------------
	void GameManager::RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx, WindowsSystem& win, InputSystem& input, PhysicsSystem& physics, AudioSystem& audio, 
		SpriteRenderSystem& renderer) {
		systems_.resourceManager_ = &rcm;
		systems_.ecs_ = &ecs;
		systems_.gfx_ = &gfx;
		systems_.win_ = &win;
		systems_.input_ = &input;
		systems_.physics_ = &physics;
		systems_.audio_ = &audio;
		systems_.spriteRenderer_ = &renderer;
		std::cout << "[GameManager] Successfully linked to all core systems" << std::endl;
	}

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
		SceneManager& sceneMgr = SceneManager::GetInstance();

		// Testing Scene Creation - Hard Coded at the moment
		SceneGUID splashScreen = sceneMgr.CreateSceneAsset("Splash Screen");
		SceneGUID testScene = sceneMgr.CreateSceneAsset("Test Scene");

		RegisterScriptToScene(splashScreen, SplashScreen::Initialize, SplashScreen::Update, SplashScreen::Free);
		RegisterScriptToScene(testScene, TestScene::Initialize, TestScene::Update, TestScene::Free);

		sceneCallbacks_[testScene].fpInitialize();
		Logger::Get().Info(DebugType::GameManager) << "Default Scene: " << sceneMgr.GetSceneAt(testScene).name << ", GUID: " << testScene << std::endl;
		sceneMgr.LoadScene(sceneMgr.GetSceneAt(testScene));

		// --- NEW: IMGUI INITIALIZATION ---
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking;     
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled, we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup Platform/Renderer backends
		ImGui_ImplGlfw_InitForOpenGL(systems_.win_->window(), true);
		ImGui_ImplOpenGL3_Init("#version 330");
	}

	void GameManager::Update(float dt) {
		deltaTime = dt;

		//  --- START THE IMGUI FRAME ---
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// --- GAME UPDATE HERE ---
		// Hard Coded to load the Test Scene as we'll be doing testing there first.
		SceneGUID currentSceneIndex = SceneManager::GetInstance().GetActiveScene().sceneIndex;
		if (sceneCallbacks_.count(currentSceneIndex) && sceneCallbacks_[currentSceneIndex].fpUpdate) {
			sceneCallbacks_[currentSceneIndex].fpUpdate();
		}

		systems_.spriteRenderer_->Update(dt);
		// --- END GAME UPDATE ---

		// --- RENDER RUNTIME STATS USING IMGUI ---
		ImGui::Begin("Game Info");
		ImGui::Text("--- Current Game Information ---");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		// --- RENDER THE IMGUI UI ---
		// This draws the UI on top of the game world.
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void GameManager::Shutdown() {
		// Call the free function for the current scene before shutting down.
		SceneGUID currentSceneIndex = SceneManager::GetInstance().GetActiveScene().sceneIndex;
		if (sceneCallbacks_.count(currentSceneIndex) && sceneCallbacks_[currentSceneIndex].fpFree) {
			sceneCallbacks_[currentSceneIndex].fpFree();
		}

		// Renderer Cleanup
		systems_.spriteRenderer_->Shutdown();
		delete systems_.spriteRenderer_;
		systems_.spriteRenderer_ = nullptr;

		instance = nullptr; // Reset instance pointer
	}

}