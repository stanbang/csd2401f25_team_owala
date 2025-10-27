/*****************************************************************//**
 * \file	TestScene.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	20 September 2025
 *********************************************************************/
#include "GameManager.h"
#include "LoadedScene.h"
#include "ScalingPlatformSystem.h"
#include "TrapPlatformSystem.h"
#include "EntityManager.h"


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

namespace LoadedScene {
    using namespace Framework;

    void Initialize() {
        SceneManager& sceneMgr = SceneManager::GetInstance();
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
        physics->SetLayerCollision(CollisionLayer::Platform, CollisionLayer::Platform, false);

        // --- Load Assets ---
        // Load necessary files into the resource manager
        rcm_->loadTexture("playerSprite", "Assets/Sprites/player_sprite_PH.png");
        rcm_->loadTexture("playerSprite_Idle", "Assets/Sprites/player_Idle_Anim.png");
        rcm_->loadTexture("playerSprite_Run", "Assets/Sprites/player_Run_Anim.png");
        rcm_->loadTexture("playerSprite_Jump", "Assets/Sprites/player_Jump_Anim.png");
        rcm_->loadAnimationInfo("playerSprite_Idle", "Data/playerIdleAnimation.json");
        rcm_->loadAnimationInfo("playerSprite_Run", "Data/playerRunningAnimation.json");
        rcm_->loadAnimationInfo("playerSprite_Jump", "Data/playerJumpingAnimation.json");
        rcm_->loadTexture("RonaldSprite_Platform", "Assets/Sprites/Platform.png");
        rcm_->loadTexture("Background", "Assets/Sprites/Env_Level2_PH.png");
        rcm_->loadSound("bgm", "Assets/Audio/bgm.wav", true, true); // Background Music
        rcm_->loadFont("amethysta_font", "Assets/Fonts/Amethysta-Regular.ttf");
        rcm_->loadFont("kiranghaerang_font", "Assets/Fonts/KirangHaerang-Regular.ttf");
        rcm_->loadSound("scream", "Assets/Audio/jingle.ogg", false, true); //Jingle SFX
        rcm_->loadSound("jump", "Assets/Audio/jump.ogg", false, false);
        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs

        // -- Create Camear Entity that follows player
        Entity mainCamera = cameraSystem->CreateCamera({ 0.0f, 0.0f }, 0, true);
        sceneMgr.AddEntityToScene("Test Scene", mainCamera);

        //set camera to follow player
        cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);

        // SET THE CAMERA SYSTEM ON THE SPRITE RENDERER
        if (spriteRenderer && cameraSystem) {
            spriteRenderer->SetCameraSystem(cameraSystem);
        }
        Logger::Get().Info(DebugType::Logger) << "TESTING: LOADED SCENE INIT" << std::endl;
    }

    void Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs_ = gm.GetSystems().ecs_;
        InputSystem* input_ = gm.GetSystems().input_;
        PhysicsSystem* physics_ = gm.GetSystems().physics_;
        //Graphics* gfx_ = gm.GetSystems().gfx_;
        ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;
        AudioSystem* audio_ = gm.GetSystems().audio_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
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
        if (input_->KeyPressed(InputSystem::Key::K)) {
            audio_->setMasterVolume(0.35f);
        }
        if (input_->KeyPressed(InputSystem::Key::J)) {
            Entity newEntity = entityMgr.CloneEntity(playerEntity);
            SceneManager::GetInstance().AddEntityToScene(SceneManager::GetInstance().GetActiveScene().name, newEntity);
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
        ////press arrow up/ arrow down to scale player sprite
        //if (input_->KeyPressed(InputSystem::Key::Up) || input_->KeyPressed(InputSystem::Key::Down)) {
        //    //GameManager& gm = GameManager::GetInstance();
        //    EntityStorage* ecs = gm.GetInstance().GetSystems().ecs_;
        //    float scale = input_->KeyPressed(InputSystem::Key::Up) ? 1.05f : (1.0f / 1.05f);
        //    updatePlayerSize(*ecs, playerEntity, scale);
        //    Logger::Get().Info(DebugType::PlayerSystem) << "Player Size *= (" << scale << ')' << std::endl;
        //}

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


        // === ADD CAMERA CONTROLS ===

        // Press 'Z' to zoom in
        if (input_->KeyPressed(InputSystem::Key::Z)) {
            Entity activeCamera = cameraSystem->GetActiveCamera();
            if (activeCamera != 0) {
                Camera& camera = entityMgr.GetComponent<Camera>(activeCamera);
                cameraSystem->SetZoom(activeCamera, camera.zoom * 0.8f); // Zoom in
            }
        }

        // Press 'X' to zoom out
        if (input_->KeyPressed(InputSystem::Key::X)) {
            Entity activeCamera = cameraSystem->GetActiveCamera();
            if (activeCamera != 0) {
                Camera& camera = entityMgr.GetComponent<Camera>(activeCamera);
                cameraSystem->SetZoom(activeCamera, camera.zoom * 1.2f); // Zoom out
            }
        }

        // Press 'R' to reset camera zoom
        if (input_->KeyPressed(InputSystem::Key::R)) {
            Entity activeCamera = cameraSystem->GetActiveCamera();
            if (activeCamera != 0) {
                cameraSystem->SetZoom(activeCamera, 1.0f); // Reset zoom
            }
        }
        float dt = gm.GetDeltaTime();
        // Update scaling platforms
        ScalingPlatformSystem scaleTrapSystem(*ecs_);
        scaleTrapSystem.Update(dt);
        // Update trap platforms
        TrapPlatformSystem tempTrapSystem(*ecs_);
        tempTrapSystem.Update(dt);
    }

    void Free() {
        Logger::Get().Info(DebugType::Logger) << "TESTING: LOADED SCENE FREE" << std::endl;
    }
}