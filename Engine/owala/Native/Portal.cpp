#include "Portal.h"
#include "ScriptRegistry.h"

// Dependencies
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
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

void Portal::SearchForSaveManager() {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());

	// Search for SaveManager
	const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
	for (Entity entity : persistents) {
		if (!entityMgr.ValidateEntity(entity)) continue;
		if (save = entityMgr.GetScript<SaveManager>(entity)) {
			return;
		}
	}
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
	SceneManager& sceneMgr = SceneManager::GetInstance();

	Entity self = GetEntity();

	if (player == 0 || !entityMgr.ValidateEntity(player)) SearchForPlayer();
	if (!save || !entityMgr.ValidateEntity(save->GetEntity())) {
		SearchForSaveManager();
		return;
	}

	if (physicsSystem.CheckTriggerOverlap(player, self)) {
		GameManager::GetInstance().GetSystems().levelResetter_->RunReset();
		audioSystem.stopAll();
		save->Unlock(levelIndexToUnlock);
		SceneManager::GetInstance().LoadScene(SceneManager::GetInstance().GetSceneByName("MainMenu"));
	}
}

void Portal::OnDestroy() {
	// Logic to be called before Entity is deleted
}
