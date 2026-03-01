#include "Portal.h"
#include "ScriptRegistry.h"

// Dependencies
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "InputSystem.h"
#include "OwalaTime.h"
#include "Vector2D.h"
#include "TimerSystem.h"  // ADD THIS INCLUDE
#include "MessageBus.h"
#include <string>

REGISTER_SCRIPT(Portal, "Portal")

void Portal::SearchForPlayer() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());

	std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);
	for (Entity entity : sceneEntities) {
		if (entityMgr.HasComponent(entity, PlayerTagType)) {
			player = entity;
			return;
		}
	}
}

void Portal::SearchForPortalChildren() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());

	// Search for other entities tied to the portal
	std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);
	for (Entity entity : sceneEntities) {
		if (!entityMgr.HasComponent(entity, TextComponentType)) continue;
		if (entityMgr.IsActive(entity) && (entityMgr.GetProperties(entity).name == "PortalText")) {
			portalText = entity;
			return;
		}
	}
}

void Portal::SearchForSaveManager() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());

	// Search for SaveManager
	const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
	for (Entity entity : persistents) {
		if (!entityMgr.ValidateEntity(entity)) continue;
		if ((save = entityMgr.GetScript<SaveManager>(entity))) {
			return;
		}
	}
}

void Portal::UpdateChildren(EntityManager& entityMgr) {
	if (!entityMgr.HasComponent(player, TransformType) && !entityMgr.HasComponent(portalText, TransformType)
		&& !entityMgr.HasComponent(portalText, TextComponentType)) return;

	Vector2D& playerPos = entityMgr.GetComponent<Transform>(player).position;
	Vector2D& portalPos = entityMgr.GetComponent<Transform>(GetEntity()).position;

	float distance = playerPos.Distance(portalPos);
	float maxDistance = 200.f;
	float alpha = 0.f;

	// If player is within 200 units and rotation is upright, activate the portal text.
	if (distance < maxDistance) {
		alpha = 1.f - (distance / maxDistance);
	}

	TextComponent& text = entityMgr.GetComponent<TextComponent>(portalText);
	text.color.a = alpha;
}

void Portal::OnCreate() {
	// Cache the levelIndex to unlock depending on the current level the portal is in.
	std::string sceneName = SceneManager::GetInstance().GetActiveScene().name;

	if (sceneName == "Tutorial 2") {
		levelIndexToUnlock = 3;
	}
	else if (sceneName == "Level 1") {
		levelIndexToUnlock = 4;
	}
	else {
		levelIndexToUnlock = 2;
	}
}

void Portal::OnUpdate() {
	GameManager& gm = GameManager::GetInstance();
	PhysicsSystem& physicsSystem = *gm.GetSystems().physics_;
	AudioSystem& audioSystem = *gm.GetSystems().audio_;
	EntityManager entityMgr(*gm.GetSystems().ecs_);

	Entity self = GetEntity();

	if (player == 0 || !entityMgr.ValidateEntity(player)) SearchForPlayer();
	if (portalText == 0 || !entityMgr.ValidateEntity(portalText)) SearchForPortalChildren();
	if (!save || !entityMgr.ValidateEntity(save->GetEntity())) {
		SearchForSaveManager();
		return;
	}

	// Handle Portal Exit Text
	if (player != 0 && portalText != 0) {
		UpdateChildren(entityMgr);
	}

	if (physicsSystem.CheckTriggerOverlap(player, self)) {
		InputSystem& input = *gm.GetSystems().input_;

		// Player must hold down for 1.5 seconds to trigger completion flag.
		if (input.KeyDown(InputSystem::Key::F)) {
			interactTimer += Time::DeltaTime();

			// Get the portal's position for VFX spawning
			Vector2D position = entityMgr.GetComponent<Transform>(self).position;
			// Publish PortalExitVFX event for the portal exit VFX system to subscribe to and spawn VFX.
			MessageBus::Instance().Publish(VFXTriggerEvent{ "PortalExitVFX", true, position });

			if (interactTimer >= 1.5f) {
				interactTimer = 0.f;

				// ===== ADD THIS =====
				// Stop the timer and submit to Firebase BEFORE resetting
				if (TimerSystem::Get().IsRunning()) {
					Framework::Logger::Get().Info(DebugType::PlayerSystem)
						<< "[Portal] Level complete! Stopping timer and submitting..." << std::endl;
					TimerSystem::Get().StopAndSubmit();
				}
				// ====================

				GameManager::GetInstance().GetSystems().levelResetter_->RunReset();
				audioSystem.stopAll();
				save->Unlock(levelIndexToUnlock);
				SceneManager::GetInstance().LoadScene(SceneManager::GetInstance().GetSceneByName("MainMenu"));
			}
		} else {
			interactTimer = 0.f;
		}
	} else {
		interactTimer = 0.f;
	}
}

void Portal::OnDestroy() {
	// Logic to be called before Entity is deleted
}