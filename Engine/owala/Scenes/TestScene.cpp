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
#include "BackgroundEntity.h"
#include "TextEntity.h"
#include "EntityStorage.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"
#include "Transformation.h"
#include "AudioEntity.h"
#include "InputSystem.h"   // if you want toggles
#include "EntitySerializer.h"
#include "PrefabSystem.h"
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PhysicsSystem.h"
#include "SceneManager.h"
#include "GameManager.h"
#include "TestScene.h"
#ifdef BUILD_EDITOR
 // ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#endif

namespace {
    using namespace Framework;
    Entity playerEntity = 0;
    //Entity groundEntity = 0;
    //Entity backgroundEntity = 0;
    //Entity trapPlatform = 0;
    //Entity topGround = 0;
    //Entity leftGround = 0;
    //Entity rightGround = 0;
    //Entity downGround = 0;
    //Entity scalingPlatform = 0;
}

namespace TestScene {
	using namespace Framework;

	void Initialize() {
        //FileIOSystem fileIO;
		//SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		WindowsSystem* win_ = gm.GetSystems().win_;
		ResourceManager* rcm_ = gm.GetSystems().resourceManager_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
        SpriteRenderSystem* spriteRenderer = gm.GetSystems().spriteRenderer_; // Get sprite renderer
        PhysicsSystem* physics = gm.GetSystems().physics_; // get physics sytems
		(void)win_; // Suppress unused parameter warning if not used
		(void)audio; // Suppress unused parameter warning if not used
        PrefabSystem prefabSystem;

        // --- CONFIGURE COLLISION MATRIX ---
        // Player should collide with Platforms, but not with other Players 
        // This setup ensures that players will land on platforms, 
        // but multiple players could pass through each other without interacting, 
        // and static platforms won't needlessly check for collisions against other static platforms. 
        physics->SetLayerCollision(CollisionLayer::Player, CollisionLayer::Platform, true);
        physics->SetLayerCollision(CollisionLayer::Player, CollisionLayer::Player, false);
        physics->SetLayerCollision(CollisionLayer::Player, CollisionLayer::Default, false);

        // Platforms will collide with each other
        physics->SetLayerCollision(CollisionLayer::Platform, CollisionLayer::Platform, true);

        physics->SetLayerCollision(CollisionLayer::Player, CollisionLayer::Enemy, true); // Player can collide with Enemy
        physics->SetLayerCollision(CollisionLayer::Platform, CollisionLayer::Enemy, true); // Enemy can collide with Platform
        physics->SetLayerCollision(CollisionLayer::Enemy, CollisionLayer::Enemy, false);  // Enemies don't collide with each other

		//Load necessary files into the resource manager
        //rcm_->loadTexture("player_sprite_PH.png", "Assets/Sprites/player_sprite_PH.png");
		//rcm_->loadTexture("Eli_Idle_Spritesheet.png", "Assets/Sprites/Eli/Eli_Idle_Spritesheet.png");
		//rcm_->loadTexture("Eli_Walk_Spritesheet.png", "Assets/Sprites/Eli/Eli_Walk_Spritesheet.png");
		//rcm_->loadTexture("player_Jump_Anim.png", "Assets/Sprites/player_Jump_Anim.png");
		rcm_->loadAnimationInfo("Eli_Idle_Spritesheet.png", "Data/AnimationData/playerIdleAnimation.json"); //WE MUST PRESERVE THIS MAPPING IF WE WANT A LINK BETWEEN SPRITESHEET AND JSON!!!!!
        rcm_->loadAnimationInfo("Eli_Walk_Spritesheet.png", "Data/AnimationData/playerRunningAnimation.json");
        rcm_->loadAnimationInfo("Eli_Jump_Spritesheet.png", "Data/AnimationData/playerJumpingAnimation.json");
        rcm_->loadAnimationInfo("Eli_Falling_Spritesheet.png", "Data/AnimationData/playerFallingAnimation.json");
        //rcm_->loadTexture("Platform.png", "Assets/Sprites/Platform.png");
        //rcm_->loadTexture("Env_Level2_PH.png", "Assets/Sprites/Env_Level2_PH.png");
		//rcm_->loadSound("bgm.wav", "Assets/Audio/bgm.wav", true, true); // Background Music
        rcm_->loadFont("Amethysta-Regular.ttf", "Assets/Fonts/Amethysta-Regular.ttf");
        rcm_->loadFont("KirangHaerang-Regular.ttf", "Assets/Fonts/KirangHaerang-Regular.ttf");
        //rcm_->loadSound("jingle.ogg", "Assets/Audio/jingle.ogg", false, true); //Jingle SFX
        //rcm_->loadSound("jump.ogg", "Assets/Audio/jump.ogg", false, false);
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs
       
        // Loading Platform Prefabs
        prefabSystem.LoadPrefabsFromFile("../../Data/platformPrefab.json");

        // --- Create Background Entity ---
        //std::string backgroundID = "Background";
        //backgroundEntity = createBackground(*ecs, { 0.0f, 0.0f }, backgroundID);
        //sceneMgr.AddEntityToScene("Test Scene", backgroundEntity);

        // --- Create Ground Entity ---
        //Entity ground = prefabSystem.CreateEntityFromPrefab(*ecs, "BasicGround", { 0.0f, -0.5f });
        //sceneMgr.AddEntityToScene("Test Scene", ground);


        // Create 4 borders around the screen
        //topGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "TopBorder", { 0.0f, 1.0f });
        //sceneMgr.AddEntityToScene("Test Scene", topGround);

        //leftGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "SideBorder", { -1.0f, 0.0f });
        //sceneMgr.AddEntityToScene("Test Scene", leftGround);

        //rightGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "SideBorder", { 1.0f, 0.0f });
        //sceneMgr.AddEntityToScene("Test Scene", rightGround);

        //downGround = prefabSystem.CreatePlatformFromPrefab(*ecs, "BottomBorder", { 0.0f, -1.0f });
        //sceneMgr.AddEntityToScene("Test Scene", downGround);

        //// Scaling Platform
        //scalingPlatform = prefabSystem.CreateScalingPlatformFromPrefab(*ecs, "ScalePlatform", { 0.5f, 0.0f });
        //sceneMgr.AddEntityToScene("Test Scene", scalingPlatform);

        //// Trap Platforms
        //trapPlatform = prefabSystem.CreateTrapPlatformFromPrefab(*ecs, "FallingTrap", { 0.0f, 0.5f });        
        //sceneMgr.AddEntityToScene("Test Scene", trapPlatform);

		// --- Create Background Music Entity ---
		//Entity bgmEntity = createBGAudio(*ecs, "bgm");
		//sceneMgr.AddEntityToScene("Test Scene", bgmEntity);

        // --- Create Player Entity ---
        //playerEntity = createPlayer(*ecs, *rcm_, fileIO.LoadPlayerStats(), { 0.1f, 0.1f }, "playerSprite_Idle");
        //sceneMgr.AddEntityToScene("Test Scene", playerEntity);
        //sceneMgr.SetEntityAsPersistent(playerEntity);
        EntityManager entityMgr(*ecs);
        playerEntity = entityMgr.GetAllEntitiesWithComponent(PlayerTagType)[0];
        Entity mainCamera = entityMgr.GetAllEntitiesWithComponent(CameraType)[0];

        cameraSystem->SetActiveCamera(mainCamera);
        cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);
        spriteRenderer->SetCameraSystem(cameraSystem);


        // --- Create Text Entity ---
        //sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "StageBound", "kiranghaerang_font", { 0.0f, 0.0f, 0.0f, 1.0f }, { -0.7f, 0.85f }, 1.5f));
        //sceneMgr.AddEntityToScene("Test Scene", createText(*ecs, "Demo", "amethysta_font", { 0.0f, 0.0f, 0.0f, 1.0f }, {-0.3f, 0.85f}, 1.5f));

        // ---- AUDIO ----
        // Add AudioSystem BEFORE GameSystem in main.cpp so this is valid now.
        // BGM: loop + stream (e.g., WAV/OGG/MP3). Path must include extension.
        //FMOD::Sound& bgm = rcm_->get<FMOD::Sound>("bgm");
        //FMOD::Sound& scream = rcm_->get<FMOD::Sound>("scream");
        //audio->play("bgm", /*volume*/0.35f, /*paused*/false);
        //EntitySerializer::SerializeEntityToFile(playerEntity, *ecs, "Data/serializedPlayer.json");
	}

	void Update() {
  //      GameManager& gm = GameManager::GetInstance();
  //      EntityStorage* ecs_ = gm.GetSystems().ecs_;
		//InputSystem* input_ = gm.GetSystems().input_;
		//PhysicsSystem* physics_ = gm.GetSystems().physics_;
		////Graphics* gfx_ = gm.GetSystems().gfx_;
  //      ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;
		//AudioSystem* audio_ = gm.GetSystems().audio_;
  //      CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
		//EntityManager entityMgr(*ecs_);

  //      //  --- START THE IMGUI FRAME ---
  //      //press T to play scream, for not a entity yet, because we don't want to play it from the start
  //      if (input_->KeyPressed(InputSystem::Key::T)) {
  //          try {
  //              FMOD::Sound& scream = resourceManager_->get<FMOD::Sound>("jingle.ogg");
  //              audio_->play(scream, /*volume*/1.0f, /*paused*/false);
  //          }
  //          catch (const GameException& e) {
		//		(void)e; //Suppress unused variable warning
  //              std::fprintf(stderr, "[Audio] Jingle sound not found!\n");
  //          }
  //      }
  //      if (input_->KeyPressed(InputSystem::Key::L)) {
  //          audio_->setMasterVolume(0.0f);
  //      }
  //      if (input_->KeyPressed(InputSystem::Key::K)) {
  //          audio_->setMasterVolume(0.35f);
  //      }
  //      if (input_->KeyPressed(InputSystem::Key::J)) {
  //          Entity newEntity = entityMgr.CloneEntity(playerEntity);
  //          SceneManager::GetInstance().AddEntityToScene(SceneManager::GetInstance().GetActiveScene().name, newEntity);
  //      }

		//// Change fake player animation between IDLE and RUNNING
		//if (input_->KeyPressed(InputSystem::Key::F)) {
		//	//Get all entities with FakePlayerTag
  //          for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(FakePlayerTagType)) {
		//		AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
		//		if (animationComponent.currentState != AnimationState::RUNNING) {
		//			animationComponent.currentState = AnimationState::RUNNING;
		//			Logger::Get().Info(DebugType::PlayerSystem) << "Fake Player Animation set to RUNNING" << std::endl;
		//		}

  //              else {
		//			animationComponent.currentState = AnimationState::IDLE;
  //                  Logger::Get().Info(DebugType::PlayerSystem) << "Fake Player Animation set to IDLE" << std::endl;
  //              }
  //          }
		//}

  //      //press ESC to crash the game
  //      if (input_->KeyPressed(InputSystem::Key::Escape)) {
  //          throw GameException(DebugType::Unknown, "Intentionally crashed the game for testing");
  //      }
  //      // Press 'C' to toggle colliders - much simpler!
  //      if (input_->KeyPressed(InputSystem::Key::C)) {
  //          //GameManager& gm = GameManager::GetInstance();
  //          SpriteRenderSystem* renderer = gm.GetInstance().GetSystems().spriteRenderer_;
  //          if (renderer) {
  //              renderer->ToggleColliders(); // Just one line!
  //          }
  //      }
  //      //press arrow up/ arrow down to scale player sprite
  //      if (input_->KeyPressed(InputSystem::Key::Up) || input_->KeyPressed(InputSystem::Key::Down)) {
  //          //GameManager& gm = GameManager::GetInstance();
  //          EntityStorage* ecs = gm.GetInstance().GetSystems().ecs_;
  //          float scale = input_->KeyPressed(InputSystem::Key::Up) ? 1.05f : (1.0f / 1.05f);
  //          updatePlayerSize(*ecs, playerEntity, scale);
  //          Logger::Get().Info(DebugType::PlayerSystem) << "Player Size *= (" << scale << ')' << std::endl;
  //      }

  //      // 2. --- HANDLE INPUTS ---
  //      // 'P' key toggles the master pause state.
  //      if (input_->KeyPressed(InputSystem::Key::P)) {
  //          physics_->SetPaused(!physics_->IsPaused());
  //          if (physics_->IsPaused()) {
  //              Logger::Get().Info(DebugType::PhysicsSystem) << "Physics Paused!" << std::endl;
  //          }
  //          else if (!physics_->IsPaused()) {
  //              Logger::Get().Info(DebugType::PhysicsSystem) << "Physics UnPaused!" << std::endl;
  //          }
  //      }
  //      
  //      // 'O' key toggles the step-by-step debug mode.
  //      if (input_->KeyPressed(InputSystem::Key::O)) {
  //          physics_->ToggleStepMode();
  //          if (physics_->IsInStepMode()) {
  //              Logger::Get().Info(DebugType::PhysicsSystem) << "Step Mode Activated - Press I to step physics forward." << std::endl;
  //          }
  //          else if (!physics_->IsInStepMode()) {
  //              Logger::Get().Info(DebugType::PhysicsSystem) << "Step Mode Deactivated" << std::endl;
  //          }
  //      }

  //      // 'I' key executes a single physics step if in step mode.
  //      if (input_->KeyPressed(InputSystem::Key::I)) {
  //          physics_->ExecuteNextStep();
  //      }

  //      if (input_->KeyPressed(InputSystem::Key::N)) {
  //          using namespace SceneManagement;
  //          SceneManager& sceneMgr = SceneManager::GetInstance();

  //          SceneAsset nextScene = sceneMgr.GetActiveScene().sceneIndex == 1 ? sceneMgr.GetSceneAt(2) : sceneMgr.GetSceneByName("Test Scene");
  //          sceneMgr.LoadScene(nextScene.name);
  //      }




  //      // === ADD CAMERA CONTROLS ===

  //      // Press 'Z' to zoom in
  //      if (input_->KeyPressed(InputSystem::Key::Z)) {
  //          Entity activeCamera = cameraSystem->GetActiveCamera();
  //          if (activeCamera != 0) {
  //              Camera& camera = entityMgr.GetComponent<Camera>(activeCamera);
  //              cameraSystem->SetZoom(activeCamera, camera.zoom * 0.8f); // Zoom in
  //          }
  //      }

  //      // Press 'X' to zoom out
  //      if (input_->KeyPressed(InputSystem::Key::X)) {
  //          Entity activeCamera = cameraSystem->GetActiveCamera();
  //          if (activeCamera != 0) {
  //              Camera& camera = entityMgr.GetComponent<Camera>(activeCamera);
  //              cameraSystem->SetZoom(activeCamera, camera.zoom * 1.2f); // Zoom out
  //          }
  //      }

  //      // Press 'R' to reset camera zoom
  //      if (input_->KeyPressed(InputSystem::Key::R)) {
  //          Entity activeCamera = cameraSystem->GetActiveCamera();
  //          if (activeCamera != 0) {
  //              cameraSystem->SetZoom(activeCamera, 1.0f); // Reset zoom
  //          }
  //      }

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
        //   RenderColliders();
	}

	void Free() {
        //std::cout << "Freeing IMGUI" << std::endl;
        //ImGui_ImplOpenGL3_Shutdown();
        //ImGui_ImplGlfw_Shutdown();
        //ImGui::DestroyContext();
	}
}