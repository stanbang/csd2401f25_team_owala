/*****************************************************************//**
 * \file	TestScene.cpp
 * \brief
 *
 *
 * \author	Jason Lou
 * \date	20 September 2025
 *********************************************************************/
#include "TestScene.h"

#include "GameManager.h"
#include "PhysicsSystem.h"
#include "PrefabSystem.h"

namespace TestScene {
	using namespace Framework;
	// Track which scene was last initialized
    static std::string lastInitializedScene;

	void Initialize() {
        //FileIOSystem fileIO;
		SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		//WindowsSystem* win_ = gm.GetSystems().win_;
		//ResourceManager* rcm_ = gm.GetSystems().resourceManager_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
        //SpriteRenderSystem* spriteRenderer = gm.GetSystems().spriteRenderer_; // Get sprite renderer
        //TextRenderSystem* textRenderer = gm.GetSystems().textRenderer_; // Get text renderer
        //PhysicsSystem* physics = gm.GetSystems().physics_; // get physics sytems
		//(void)win_; // Suppress unused parameter warning if not used
		(void)audio; // Suppress unused parameter warning if not used
        // Get the current scene name to track initialization per scene
        std::string currentSceneName = sceneMgr.GetActiveScene().name;
        bool isFirstInitialization = (lastInitializedScene != currentSceneName);

        // --- CONFIGURE COLLISION MATRIX ---
        // CONFIGURED IN LAYER MANAGER
        

        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs
       
        EntityManager entityMgr(*ecs);
        
        std::vector<Entity> players = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
        Entity playerEntity = 0;
        if (players.size() > 0) {
            playerEntity = players[0];
        }
		else {
			Framework::Logger::Get().Error(DebugType::GameManager)
                << "[TestScene] Player missing! PlayerCount: "
                << players.size() << std::endl;
		}

        std::vector<Entity> cameras = entityMgr.GetAllEntitiesWithComponent(true, CameraType);
        Entity mainCamera = 0;
        if (cameras.size() > 0) {
            mainCamera = cameras[0];
            cameraSystem->SetActiveCamera(mainCamera);
            sceneMgr.SetEntityAsPersistent(mainCamera);

            // Ensure game camera is NOT editor-controlled
            Camera& gameCam = cameraSystem->GetCamera(mainCamera);
            gameCam.isEditorControlled = false;
        }

        if (playerEntity && mainCamera) {
            cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);
            cameraSystem->CalculateAndSetBoundsFromPlatforms(mainCamera, 0.0f);
        }

        // Spawn player at SpawnPoint
        if (isFirstInitialization) {
            std::vector<Entity> spawnPts = entityMgr.GetAllEntitiesWithComponents(true, SpawnPointTagType, LoadedTagType, TransformType);
            if (spawnPts.size() > 0) {
                if (entityMgr.HasComponent(playerEntity, TransformType)) {
                    Entity playerSpawnPt = spawnPts[0];
                    Transform& playerTransform = entityMgr.GetComponent<Transform>(playerEntity);
                    Transform spawnPtTransform = entityMgr.GetComponent<Transform>(playerSpawnPt);

                    playerTransform.position = spawnPtTransform.position;
                    Framework::Logger::Get().Info(DebugType::GameManager) << "[TestScene] Spawned Player at (" << playerTransform.position.x
                        << ", " << playerTransform.position.y << ")" << std::endl;
                }
                else {
                    Framework::Logger::Get().Error(DebugType::GameManager) << "[TestScene] Player does not have Transform component!" << std::endl;
                }
            }
            else {
                Framework::Logger::Get().Error(DebugType::GameManager)
                    << "[TestScene] Player SpawnPoint missing! SpawnPtCount: "
                    << spawnPts.size() << std::endl;
            }

            // Mark this scene as initialized
            lastInitializedScene = currentSceneName;
        }
	}

	void Update() {
        //GameManager& gm = GameManager::GetInstance();
        //PhysicsSystem& physicsSystem = *gm.GetSystems().physics_;
        //AudioSystem& audioSystem = *gm.GetSystems().audio_;
        //EntityManager entityMgr(*gm.GetSystems().ecs_);

        //// Get all entities in scene
        //std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);

        //// Find the player in list of active entities.
        //Entity player = 0;
        //for (Entity entity : sceneEntities) {
        //    if (entityMgr.HasComponent(entity, PlayerTagType)) {
        //        player = entity;
        //        break;
        //    }
        //}

        //if (player == 0) return;

        //// Find the door in list of active entities.
        //Entity portal = 0;
        //for (Entity entity : sceneEntities) {
        //    if (!entityMgr.HasComponent(entity, AABBColliderType)) continue;

        //    if (entityMgr.GetProperties(entity).name == "Portal") {
        //        portal = entity;
        //        break;
        //    }
        //}

        //if (portal == 0) return;

        //if (physicsSystem.CheckTriggerOverlap(player, portal)) {
        //    GameManager::GetInstance().GetSystems().levelResetter_->RunReset();
        //    audioSystem.stopAll();
        //    SceneManager::GetInstance().LoadScene(SceneManager::GetInstance().GetSceneByName("MainMenu"));
        //}
	}

	void Free() {
	}
}