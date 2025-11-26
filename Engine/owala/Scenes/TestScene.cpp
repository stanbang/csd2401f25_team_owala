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
		WindowsSystem* win_ = gm.GetSystems().win_;
		//ResourceManager* rcm_ = gm.GetSystems().resourceManager_;
        CameraSystem* cameraSystem = gm.GetSystems().camera_;  //GET CAMERA SYSTEM
        SpriteRenderSystem* spriteRenderer = gm.GetSystems().spriteRenderer_; // Get sprite renderer
        TextRenderSystem* textRenderer = gm.GetSystems().textRenderer_; // Get text renderer
        PhysicsSystem* physics = gm.GetSystems().physics_; // get physics sytems
		(void)win_; // Suppress unused parameter warning if not used
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
        std::vector<Entity> cameras = entityMgr.GetAllEntitiesWithComponent(CameraType);

        // Check if we actually found a player and a camera
        if (!players.empty() && !cameras.empty()) {
            Entity playerEntity = players[0];
            Entity mainCamera = cameras[0];

            if (cameraSystem) {
                cameraSystem->SetActiveCamera(mainCamera);
                cameraSystem->SetFollowTarget(mainCamera, playerEntity, 8.0f);
            }
        }
        else {
            // Log warning if entities are missing so you know why the camera isn't moving
            Framework::Logger::Get().Error(DebugType::GameManager)
                << "[TestScene] Player or Camera missing! PlayerCount: "
                << players.size() << ", CameraCount: " << cameras.size() << std::endl;
        }

        // Set system cameras (safe to do even if no active camera entity found yet)
        if (cameraSystem) {
            spriteRenderer->SetCameraSystem(cameraSystem);
            textRenderer->SetCameraSystem(cameraSystem);
        }
	}

	void Update() {
	}

	void Free() {
	}
}