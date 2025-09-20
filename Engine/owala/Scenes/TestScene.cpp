/*****************************************************************//**
 * \file	TestScene.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	20 September 2025
 *********************************************************************/
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Player.h"
#include "Platform.h"
#include "EntityStorage.h"
#include "RotationSystem.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"

#include "SpriteComponents.h"
#include "InputSystem.h"   // if you want toggles
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PhysicsSystem.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "TestScene.h"

namespace {
    using namespace Framework;
    Entity playerEntity = 0;
	Entity groundEntity = 0;
}

namespace TestScene {
	using namespace Framework;

	void Initialize() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;

        // --- Load Assets ---
        // Ensure you have these files in an "Assets" folder next to your executable.
        //playerTex_ = gfx_->loadTextureFromFile("Assets/duck-rgba-256.tex");
        //groundTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png");
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs

        // --- Create Ground Entity ---
        std::string platformTexID = "RonaldSprite_Platform";
        groundEntity = createPlatform(*ecs, { 0.0f, -0.5f }, { 1.0f, 0.2f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", groundEntity);

        // --- Create Player Entity ---
        FileIOSystem fileIO;
        PlayerStats stats = fileIO.loadPlayerStats(); // Example stats from file loading

        std::string playerDuckID = "playerSprite_DuckyWucky";
        playerEntity = createPlayer(*ecs, stats, { 0.1f, 0.1f }, playerDuckID);
        sceneMgr.AddEntityToScene("Test Scene", playerEntity);
        sceneMgr.SetEntityAsPersistent(playerEntity);

        // ---- AUDIO ----
        // Add AudioSystem BEFORE GameSystem in main.cpp so this is valid now.
        // BGM: loop + stream (e.g., WAV/OGG/MP3). Path must include extension.
        bool ok_bgm = audio->loadSound(
            "bgm",
            "../Assets/siren.ogg",   // <--- ensure this file exists
            /*loop*/ true,
            /*stream*/ true
        );
        if (!ok_bgm) {
            std::fprintf(stderr, "[Audio] Failed to load bgm\n");
        }
        audio->play("bgm", /*volume*/0.35f, /*paused*/false);
	}
	void Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs_ = gm.GetSystems().ecs_;
		InputSystem* input_ = gm.GetSystems().input_;
		PhysicsSystem* physics_ = gm.GetSystems().physics_;
		Graphics* gfx_ = gm.GetSystems().gfx_;
        ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;

        // 'P' key toggles the master pause state.
        if (input_->KeyPressed(InputSystem::Key::P)) {
            physics_->SetPaused(!physics_->IsPaused());
        }

        // 'O' key toggles the step-by-step debug mode.
        if (input_->KeyPressed(InputSystem::Key::O)) {
            physics_->ToggleStepMode();
        }

        // 'I' key executes a single physics step if in step mode.
        if (input_->KeyPressed(InputSystem::Key::I)) {
            physics_->ExecuteNextStep();
        }

        if (input_->KeyPressed(InputSystem::Key::N)) {
            using namespace SceneManagement;
            SceneManager& sceneMgr = SceneManager::GetInstance();

            SceneAsset nextScene = sceneMgr.GetActiveScene().sceneIndex == 1 ? sceneMgr.GetSceneAt(2) : sceneMgr.GetSceneByName("Test Scene");
            sceneMgr.LoadScene(nextScene.name);
        }

        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });


        // Render the ground

        if (ecs_->tex.count(groundEntity) && ecs_->position.count(groundEntity)) {
            const auto& pos = ecs_->position.at(groundEntity).Pos;
            const auto& halfSize = ecs_->colliders.at(groundEntity).HalfSize;
            float rotation = 0.0f;
            if (ecs_->rotationFlag.count(groundEntity)) {
                rotation = ecs_->rotationFlag.at(groundEntity).currentRotation * (M_PI / 180.0f);
            }

            //Use the textureID to get the actual texture from the resource manager
            std::string textureID = ecs_->tex[groundEntity].textureKey;
            Texture texture = resourceManager_->get<Texture>(textureID);
            gfx_->drawSprite(texture, pos, halfSize, rotation);
        }

        // Render the player
        if (ecs_->tex.count(playerEntity) && ecs_->position.count(playerEntity)) {
            const auto& pos = ecs_->position.at(playerEntity).Pos;
            const auto& halfSize = ecs_->colliders.at(playerEntity).HalfSize;


            std::string textureID = ecs_->tex[playerEntity].textureKey;
            //Use the textureID to get the actual texture from the resource manager
            Texture texture = resourceManager_->get<Texture>(textureID);
            gfx_->drawSprite(texture, pos, halfSize, 0.0f);
        }

        // For now, we can use the console to see the state.
        if (physics_->IsInStepMode()) {

            std::cout << "STEP MODE ACTIVE (Press 'I' to advance)" << std::endl;
        }
        else if (physics_->IsPaused()) {
            std::cout << "PAUSED" << std::endl;
        }
	}
	void Free() {
		// Empty by design
	}
}