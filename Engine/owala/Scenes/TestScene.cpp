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
#include "TextEntity.h"
#include "EntityStorage.h"
#include "RotationSystem.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"
#include "TrapPlatform.h"
#include "Transformation.h"
#include "ScalingPlatform.h"
#include "AudioEntity.h"
#include <PlatformPrefab.h>
#include <TrapPlatformSystem.h>
#include <ScalingPlatformSystem.h>
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
    Entity trapPlatform = 0;
    Entity topGround = 0;
    Entity leftGround = 0;
    Entity rightGround = 0;
    Entity downGround = 0;
    Entity scalingPlatform = 0;
}

namespace TestScene {
	using namespace Framework;

	void Initialize() {
        FileIOSystem fileIO;
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		WindowsSystem* win_ = gm.GetSystems().win_;
		ResourceManager* rcm_ = gm.GetSystems().resourceManager_;
		(void)win_; // Suppress unused parameter warning if not used
		(void)audio; // Suppress unused parameter warning if not used
        PrefabSystem prefabSystem;

        // --- Load Assets ---
		// Load necessary files into the resource manager
        rcm_->loadTexture("playerSprite", "Assets/player_sprite_PH.png");
		rcm_->loadTexture("playerSprite_Idle", "Assets/player_idle_animation.png");
        rcm_->loadAnimationInfo("playerSprite_Idle", "Data/playerIdleAnimation.json");
        rcm_->loadTexture("RonaldSprite_Platform", "Assets/Ronald.png");
        rcm_->loadTexture("Background", "Assets/Env_Level2_PH.png");
		rcm_->loadSound("bgm", "Assets/siren.wav", true, true); // siren sound for relaxing background music
        rcm_->loadFont("amethysta_font", "Assets/Fonts/Amethysta-Regular.ttf");
        rcm_->loadFont("kiranghaerang_font", "Assets/Fonts/KirangHaerang-Regular.ttf");
        rcm_->loadSound("scream", "Assets/Goat.ogg", false, true); //scream sound effect
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs
       
        // Loading Platform Prefabs
        prefabSystem.LoadPrefabsFromFile("Data/platformPrefab.txt");

        // --- Create Background Entity ---
        std::string backgroundID = "Background";
        backgroundEntity = createBackground(*ecs, { 0.0f, 0.0f }, backgroundID);
        sceneMgr.AddEntityToScene("Test Scene", backgroundEntity);

        // --- Create Ground Entity ---
        Entity ground = prefabSystem.CreatePlatformFromPrefab(*ecs, "BasicGround", { 0.0f, -0.5f });
        sceneMgr.AddEntityToScene("Test Scene", ground);


        // Create 4 borders around the screen
        topGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "TopBorder", { 0.0f, 1.0f });
        sceneMgr.AddEntityToScene("Test Scene", topGround);

        leftGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "SideBorder", { -1.0f, 0.0f });
        sceneMgr.AddEntityToScene("Test Scene", leftGround);

        rightGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "SideBorder", { 1.0f, 0.0f });
        sceneMgr.AddEntityToScene("Test Scene", rightGround);

        downGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "BottomBorder", { 0.0f, -1.0f });
        sceneMgr.AddEntityToScene("Test Scene", downGround);

        // Scaling Platform
        scalingPlatform = prefabSystem.CreateScalingPlatformFromPrefab(*ecs, "ScalePlatform", { -0.5f, 0.0f });
        sceneMgr.AddEntityToScene("Test Scene", scalingPlatform);

        // Trap Platforms
        trapPlatform = prefabSystem.CreateTrapPlatformFromPrefab(*ecs, "FallingTrap", { 0.0f, 0.5f });        
        sceneMgr.AddEntityToScene("Test Scene", trapPlatform);

		// --- Create Background Music Entity ---
		Entity bgmEntity = createBGAudio(*ecs, "bgm");
		sceneMgr.AddEntityToScene("Test Scene", bgmEntity);

        // --- Create Player Entity ---
        playerEntity = createPlayer(*ecs, *rcm_, fileIO.LoadPlayerStats(), { 0.07f, 0.2f }, "playerSprite_Idle");
        sceneMgr.AddEntityToScene("Test Scene", playerEntity);
        sceneMgr.SetEntityAsPersistent(playerEntity);

        // --- Create Text Entity ---
        sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "StageBound", "kiranghaerang_font", { 0.0f, 0.0f, 0.0f, 1.0f }, { -0.7f, 0.85f }, 1.5f));
        sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "Demo", "amethysta_font", { 0.0f, 0.0f, 0.0f, 1.0f }, {-0.3f, 0.85f}, 1.5f));

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
		//Graphics* gfx_ = gm.GetSystems().gfx_;
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
				(void)e; //Suppress unused variable warning
                std::fprintf(stderr, "[Audio] Scream sound not found!\n");
            }
        }
        if (input_->KeyPressed(InputSystem::Key::L)) {
            audio_->setMasterVolume(0.0f);
        }

        //press ESC to crash the game
        if (input_->KeyPressed(InputSystem::Key::Escape)) {
            throw GameException(DebugType::Unknown, "Intentionally crashed the game for testing");
        }
        // Press 'C' to toggle colliders - much simpler!
        if (input_->KeyPressed(InputSystem::Key::C)) {
            //GameManager& gm = GameManager::GetInstance();
            SpriteRenderSystem* renderer = gm.GetInstance().GetSystems().spriteRenderer_;
            if (renderer) {
                renderer->ToggleColliders(); // Just one line!
            }
        }
        //press arrow up/ arrow down to scale player sprite
        if (input_->KeyPressed(InputSystem::Key::Up) || input_->KeyPressed(InputSystem::Key::Down)) {
            //GameManager& gm = GameManager::GetInstance();
            EntityStorage* ecs = gm.GetInstance().GetSystems().ecs_;
            float scale = input_->KeyPressed(InputSystem::Key::Up) ? 1.05f : (1.0f / 1.05f);
            updatePlayerSize(*ecs, playerEntity, scale);
            Logger::Get().Info(DebugType::PlayerSystem) << "Player Size *= (" << scale << ')' << std::endl;
        }

        // 2. --- HANDLE INPUTS ---
        // 'P' key toggles the master pause state.
        if (input_->KeyPressed(InputSystem::Key::P)) {
            physics_->SetPaused(!physics_->IsPaused());
            if (physics_->IsPaused()) {
                Logger::Get().Info(DebugType::PhysicsSystem) << "Physics Paused!" << std::endl;
            }
            else if (!physics_->IsPaused()) {
                Logger::Get().Info(DebugType::PhysicsSystem) << "Physics UnPaused!" << std::endl;
            }
        }
        
        // 'O' key toggles the step-by-step debug mode.
        if (input_->KeyPressed(InputSystem::Key::O)) {
            physics_->ToggleStepMode();
            if (physics_->IsInStepMode()) {
                Logger::Get().Info(DebugType::PhysicsSystem) << "Step Mode Activated - Press I to step physics forward." << std::endl;
            }
            else if (!physics_->IsInStepMode()) {
                Logger::Get().Info(DebugType::PhysicsSystem) << "Step Mode Deactivated" << std::endl;
            }
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

        //// 3. --- BUILD IMGUI INTERFACE ---
        //// Example of a dockable "Level Editor" window.
        //// Create the main dockspace that will fill the entire game window.
        //const ImGuiViewport* viewport = ImGui::GetMainViewport();
        //ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        //ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        //ImGui::Begin("Level Editor");

        //    if (ImGui::CollapsingHeader("Player Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {
        //        // Check if the player entity and its components exist.
        //        if (entityMgr.HasComponent(playerEntity, PositionType) && entityMgr.HasComponent(playerEntity, VelocityType) && entityMgr.HasComponent(playerEntity, PlayerStateType)) {

        //            // Get references to the player's components.
        //            Vector2D& pos = entityMgr.GetComponent<Position>(playerEntity).Pos;
        //            Vector2D& vel = entityMgr.GetComponent<Velocity>(playerEntity).Vel;
        //            PlayerState& state = entityMgr.GetComponent<PlayerState>(playerEntity);

        //            // Create ImGui widgets linked to the component data.
        //            ImGui::DragFloat2("Position", &pos.x, 0.01f);
        //            ImGui::InputFloat2("Velocity", &vel.x, "%.2f", ImGuiInputTextFlags_ReadOnly);
        //            ImGui::Checkbox("Is Grounded?", &state.isGrounded);

        //            // A button to apply an action to the entity.
        //            if (ImGui::Button("Reset Player Position")) {
        //                pos = { 0.0f, 0.5f };
        //                vel = { 0.0f, 0.0f };
        //            }
        //        }
        //        else {
        //            ImGui::Text("Player entity data not found!");
        //        }
        //    }

        //    if (ImGui::CollapsingHeader("Global Controls")) {
        //        static float masterVolume = 0.35f;
        //        if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f)) {
        //            audio_->setMasterVolume(masterVolume);
        //        }
        //    }

        //    ImGui::End(); // End "Level Editor" window

            // You can also show the ImGui demo window to explore widgets.
           //  ImGui::ShowDemoWindow(); 

        float dt = gm.GetDeltaTime();
        // Update scaling platforms
        ScalingPlatformSystem scaleTrapSystem(*ecs_);
        scaleTrapSystem.Update(dt);
        // Update trap platforms
        TrapPlatformSystem tempTrapSystem(*ecs_);
        tempTrapSystem.Update(dt);
        //   RenderColliders();
	}

	void Free() {
        //std::cout << "Freeing IMGUI" << std::endl;
        //ImGui_ImplOpenGL3_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        //ImGui::DestroyContext();
	}
}