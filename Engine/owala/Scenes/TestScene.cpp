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

	void Initialize() {
        //FileIOSystem fileIO;
		//SceneManager& sceneMgr = SceneManager::GetInstance();
		GameManager& gm = GameManager::GetInstance();
		EntityStorage* ecs = gm.GetSystems().ecs_;
		AudioSystem* audio = gm.GetSystems().audio_;
		//WindowsSystem* win_ = gm.GetSystems().win_;
		//ResourceManager* rcm_ = gm.GetSystems().resourceManager_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
        //SpriteRenderSystem* spriteRenderer = gm.GetSystems().spriteRenderer_; // Get sprite renderer
        //TextRenderSystem* textRenderer = gm.GetSystems().textRenderer_; // Get text renderer
        PhysicsSystem* physics = gm.GetSystems().physics_; // get physics sytems
		//(void)win_; // Suppress unused parameter warning if not used
		(void)audio; // Suppress unused parameter warning if not used

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

        gm.GetSystems().gfx_->setAlphaBlend(true); // Enable transparency for PNGs
       
        EntityManager entityMgr(*ecs);
        
        std::vector<Entity> players = entityMgr.GetAllEntitiesWithComponent(PlayerTagType);
        Entity playerEntity = 0;
        if (players.size() > 0) {
            playerEntity = players[0];
        }
		else {
			Framework::Logger::Get().Error(DebugType::GameManager)
                << "[TestScene] Player missing! PlayerCount: "
                << players.size() << std::endl;
		}

        std::vector<Entity> cameras = entityMgr.GetAllEntitiesWithComponent(CameraType);
        Entity mainCamera = 0;
        if (cameras.size() > 0) {
            mainCamera = cameras[0];
            cameraSystem->SetActiveCamera(mainCamera);
            SceneManager::GetInstance().SetEntityAsPersistent(mainCamera);

            // Ensure game camera is NOT editor-controlled
            Camera& gameCam = cameraSystem->GetCamera(mainCamera);
            gameCam.isEditorControlled = false;
        }

        if (playerEntity && mainCamera) {
            cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);
            cameraSystem->CalculateAndSetBoundsFromPlatforms(mainCamera, 0.0f);

        }
	}

	void Update() {
        GameManager& gm = GameManager::GetInstance();
        PhysicsSystem& physicsSystem = *gm.GetSystems().physics_;
        AudioSystem& audioSystem = *gm.GetSystems().audio_;
        EntityManager entityMgr(*gm.GetSystems().ecs_);

        // Get all entities in scene
        std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);

        // Find the player in list of active entities.
        Entity player = 0;
        for (Entity entity : sceneEntities) {
            if (entityMgr.HasComponent(entity, PlayerTagType)) {
                player = entity;
                break;
            }
        }

        if (player == 0) return;

        // Find the door in list of active entities.
        Entity portal = 0;
        for (Entity entity : sceneEntities) {
            if (!entityMgr.HasComponent(entity, AABBColliderType)) continue;

            if (entityMgr.GetProperties(entity).name == "Portal") {
                portal = entity;
                break;
            }
        }

        if (portal == 0) return;

        if (physicsSystem.CheckTriggerOverlap(player, portal)) {
            GameManager::GetInstance().GetSystems().levelResetter_->RunReset();
            audioSystem.stopAll();
            SceneManager::GetInstance().LoadScene(SceneManager::GetInstance().GetSceneByName("MainMenu"));
        }
	}

	void Free() {
	}
}