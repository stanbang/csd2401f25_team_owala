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

namespace Framework {
	// ---------------------------------------------------------------------------
	// Singleton Stuff
	// ---------------------------------------------------------------------------
	GameManager* GameManager::instance = nullptr; // Initialize the static instance pointer to nullptr
	
	GameManager& GameManager::GetInstance() {
		if (!instance) {
			instance = new GameManager();
			std::cout << "[GameManager] GameManager Instantiated!" << std::endl;
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
	void GameManager::CheckSystems() {
		if (!systems_.resourceManager_) {
			throw std::runtime_error("[GameManager] ResourceManager not registered!");
		}
		if (!systems_.ecs_) {
			throw std::runtime_error("[GameManager] EntityStorage not registered!");
		}
		if (!systems_.gfx_) {
			throw std::runtime_error("[GameManager] Graphics not registered!");
		}
		if (!systems_.win_) {
			throw std::runtime_error("[GameManager] WindowsSystem not registered!");
		}
		if (!systems_.input_) {
			throw std::runtime_error("[GameManager] InputSystem not registered!");
		}
		if (!systems_.physics_) {
			throw std::runtime_error("[GameManager] PhysicsSystem not registered!");
		}
		if (!systems_.audio_) {
			throw std::runtime_error("[GameManager] AudioSystem not registered!");
		}
		if (!SceneManager::Exists()) {
			throw std::runtime_error("[GameManager] SceneManager not registered!");
		}
	}

	// ---------------------------------------------------------------------------
	// GameManager Config
	// ---------------------------------------------------------------------------
	void GameManager::RegisterSystemsToUse(ResourceManager& rcm, EntityStorage& ecs, Graphics& gfx, WindowsSystem& win, InputSystem& input, PhysicsSystem& physics, AudioSystem& audio) {
		systems_.resourceManager_ = &rcm;
		systems_.ecs_ = &ecs;
		systems_.gfx_ = &gfx;
		systems_.win_ = &win;
		systems_.input_ = &input;
		systems_.physics_ = &physics;
		systems_.audio_ = &audio;
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
		std::cout << "[GameManager] Default Scene: " << sceneMgr.GetSceneAt(testScene).name << ", GUID: " << testScene << std::endl;
		sceneMgr.LoadScene(sceneMgr.GetSceneAt(testScene));
	}

	void GameManager::Update(float dt) {
		deltaTime = dt;

		// Hard Coded to load the Test Scene as we'll be doing testing there first.
		SceneGUID currentSceneIndex = SceneManager::GetInstance().GetActiveScene().sceneIndex;
		if (sceneCallbacks_.count(currentSceneIndex) && sceneCallbacks_[currentSceneIndex].fpUpdate) {
			sceneCallbacks_[currentSceneIndex].fpUpdate();
		}
	}
}