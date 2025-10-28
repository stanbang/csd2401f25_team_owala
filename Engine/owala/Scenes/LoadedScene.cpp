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
#include "EntityManager.h"
#include "Player.h"

namespace LoadedScene {
    using namespace Framework;

    void Initialize() {
    }

    void Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityStorage* ecs_ = gm.GetSystems().ecs_;
        InputSystem* input_ = gm.GetSystems().input_;
        PhysicsSystem* physics_ = gm.GetSystems().physics_;
        ResourceManager* resourceManager_ = gm.GetSystems().resourceManager_;
        AudioSystem* audio_ = gm.GetSystems().audio_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
        EntityManager entityMgr(*ecs_);

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
            Entity playerEntity = entityMgr.GetAllEntitiesWithComponent(ComponentType::PlayerTagType)[0];
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
        //press arrow up/ arrow down to scale player sprite
        if (input_->KeyPressed(InputSystem::Key::Up) || input_->KeyPressed(InputSystem::Key::Down)) {
            //GameManager& gm = GameManager::GetInstance();
            EntityStorage* ecs = gm.GetInstance().GetSystems().ecs_;
            float scale = input_->KeyPressed(InputSystem::Key::Up) ? 1.05f : (1.0f / 1.05f);
            Entity playerEntity = entityMgr.GetAllEntitiesWithComponent(ComponentType::PlayerTagType)[0];
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
    }

    void Free() {
    }
}