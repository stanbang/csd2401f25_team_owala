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
#include "BackgroundEntity.h"
#include "Text.h"
#include "EntityStorage.h"
#include "RotationSystem.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"
#include "TrapPlatform.h"
#include "Transformation.h"
#include "ScalingPlatform.h"
#include "AudioEntity.h"

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

 // ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace {
    using namespace Framework;
    Entity playerEntity = 0;
    Entity groundEntity = 0;
    Entity backgroundEntity = 0;
    Entity platformTrap_ = 0;
    Entity topGround = 0;
    Entity leftGround = 0;
    Entity rightGround = 0;
    Entity downGround = 0;
    Entity scalingPlatform = 0;
}

namespace TestScene {
	using namespace Framework;

	void Initialize() {
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		WindowsSystem* win_ = gm.GetSystems().win_;
		ResourceManager* rcm_ = gm.GetSystems().resourceManager_;

        // --- Load Assets ---
		// Load necessary files into the resource manager
        rcm_->loadTexture("playerSprite_DuckyWucky", "Assets/duck-rgba-256.tex");
        rcm_->loadTexture("RonaldSprite_Platform", "Assets/Ronald.png");
        rcm_->loadTexture("Background", "Assets/Env_Level2_Placeholder_Upsized.png");
		rcm_->loadSound("bgm", "Assets/siren.ogg", true, true); // siren sound for relaxing background music
        rcm_->loadFont("minecraft_font", "Assets/Fonts/minecraft.ttf");
        rcm_->loadFont("kremlin_font", "Assets/Fonts/Kremlin.ttf");
        rcm_->loadSound("scream", "Assets/Goat.ogg", true, true); //scream sound effect
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs

        // --- Create Background Entity ---
        std::string backgroundID = "Background";
        backgroundEntity = createBackground(*ecs, { 0.0f, 0.0f }, backgroundID);
        sceneMgr.AddEntityToScene("Test Scene", backgroundEntity);

        // --- Create Ground Entity ---
        std::string platformTexID = "RonaldSprite_Platform";
        groundEntity = createPlatform(*ecs, { 0.0f, -0.5f }, { 1.0f, 0.2f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", groundEntity);


        // Create 4 borders around the screen
        topGround = createPlatform(*ecs, { 0.0f, 1.f }, { 1.0f, 0.2f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", topGround);

        leftGround = createPlatform(*ecs, { -1.0f, 0.0f }, { 0.2f, 1.0f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", leftGround);

        rightGround = createPlatform(*ecs, { 1.0f, 0.0f }, { 0.2f, 1.0f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", rightGround);

        downGround = createPlatform(*ecs, { 0.0f, -1.0f }, { 1.0f, 0.2f }, platformTexID);
        sceneMgr.AddEntityToScene("Test Scene", downGround);

        // Scaling Platform
        std::string scalingPlatformTexID = "RonaldSprite_Platform";
        scalingPlatform = createScalingPlatform(*ecs, { -0.5f, 0.0f },{ 0.3f, 0.1f },scalingPlatformTexID,2.0f,0.2f,1.0f);
        sceneMgr.AddEntityToScene("Test Scene", scalingPlatform);

        // Trap Platforms
        std::string trapTexID = "RonaldSprite_Platform"; 
        platformTrap_ = createTrapPlatform(*ecs, { 0.0f, 0.5f }, { 0.5f, 0.1f }, trapTexID);
        sceneMgr.AddEntityToScene("Test Scene", platformTrap_);

        // --- Create Player Entity ---
        FileIOSystem fileIO;
        PlayerStats stats = fileIO.LoadPlayerStats(); // Example stats from file loading

		// --- Create Background Music Entity ---
		Entity bgmEntity = createBGAudio(*ecs, stats, { 0.1f, 0.1f }, "bgm");
		sceneMgr.AddEntityToScene("Test Scene", bgmEntity);

        std::string playerDuckID = "playerSprite_DuckyWucky";
        playerEntity = createPlayer(*ecs, stats, { 0.1f, 0.1f }, playerDuckID);
        sceneMgr.AddEntityToScene("Test Scene", playerEntity);
        sceneMgr.SetEntityAsPersistent(playerEntity);

        // --- Create Text Entity ---
        sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "StageBound", "minecraft_font", { 1,1,1,1 }, { -0.7f, 0.8f }, 1.0f));
        sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "Demo", "kremlin_font", { 0.5,1,1,1 }, { -0.7f, 0.6f }, 1.0f));

        // ---- AUDIO ----
        // Add AudioSystem BEFORE GameSystem in main.cpp so this is valid now.
        // BGM: loop + stream (e.g., WAV/OGG/MP3). Path must include extension.
        //FMOD::Sound& bgm = rcm_->get<FMOD::Sound>("bgm");
        //FMOD::Sound& scream = rcm_->get<FMOD::Sound>("scream");
        //audio->play("bgm", /*volume*/0.35f, /*paused*/false);
	}

	void Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs_ = gm.GetSystems().ecs_;
		InputSystem* input_ = gm.GetSystems().input_;
		PhysicsSystem* physics_ = gm.GetSystems().physics_;
		Graphics* gfx_ = gm.GetSystems().gfx_;
        ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;
		AudioSystem* audio_ = gm.GetSystems().audio_;
		EntityManager entityMgr(*ecs_);

        //  --- START THE IMGUI FRAME ---
        //press T to play scream, for not a entity yet, because we don't want to play it from the start
        if (input_->KeyPressed(InputSystem::Key::T)) {
            try {
                FMOD::Sound& scream = resourceManager_->get<FMOD::Sound>("scream");
                audio_->play(scream, /*volume*/1.0f, /*paused*/false);
            }
            catch (const GameException& e) {
                std::fprintf(stderr, "[Audio] Scream sound not found!\n");
            }
        }

        //press ESC to crash the game
        if (input_->KeyPressed(InputSystem::Key::Escape)) {
            throw GameException(DebugType::Unknown, "Intentionally crashed the game for testing");
        }

        // 2. --- HANDLE INPUTS ---
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

        // 3. --- BUILD IMGUI INTERFACE ---
         // This is where you create all your ImGui windows and widgets.
        {
            // Create the main dockspace that will fill the entire game window.
            const ImGuiViewport* viewport = ImGui::GetMainViewport();

            // Example of a dockable "Level Editor" window.
            ImGui::Begin("Level Editor");

            if (ImGui::CollapsingHeader("Player Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Check if the player entity and its components exist.
                if (entityMgr.HasComponent(playerEntity, PositionType) && entityMgr.HasComponent(playerEntity, VelocityType) && entityMgr.HasComponent(playerEntity, PlayerStateType)) {

                    // Get references to the player's components.
                    Vector2D& pos = entityMgr.GetComponent<Position>(playerEntity).Pos;
                    Vector2D& vel = entityMgr.GetComponent<Velocity>(playerEntity).Vel;
                    PlayerState& state = entityMgr.GetComponent<PlayerState>(playerEntity);

                    // Create ImGui widgets linked to the component data.
                    ImGui::DragFloat2("Position", &pos.x, 0.01f);
                    ImGui::InputFloat2("Velocity", &vel.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
                    ImGui::Checkbox("Is Grounded?", &state.isGrounded);

                    // A button to apply an action to the entity.
                    if (ImGui::Button("Reset Player Position")) {
                        pos = { 0.0f, 0.5f };
                        vel = { 0.0f, 0.0f };
                    }
                }
                else {
                    ImGui::Text("Player entity data not found!");
                }
            }

            if (ImGui::CollapsingHeader("Global Controls")) {
                static float masterVolume = 0.35f;
                if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
                    audio_->setMasterVolume(masterVolume);
                }
            }

            ImGui::End(); // End "Level Editor" window

            // You can also show the ImGui demo window to explore widgets.
            // ImGui::ShowDemoWindow(); 
        }

        // Update trap platforms
        float dt = gm.GetDeltaTime();
        updateTrapPlatforms(*ecs_, dt);
        triggerTrapPlatform(*ecs_, platformTrap_);
        // Update scaling platforms
        updateScalingPlatforms(*ecs_, dt);

        //   RenderColliders();
	}

	void Free() {
        std::cout << "Freeing IMGUI" << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
	}
}