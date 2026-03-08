/*****************************************************************//**
 * \file	PlayerHealthHUD.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author

 *
 * \brief	Contains definitions for the PlayerHealthHUD script
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "PlayerHealthHUD.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(PlayerHealthHUD, "PlayerHealthHUD")

void PlayerHealthHUD::OnCreate() {
	self_ = GetEntity();
}

void PlayerHealthHUD::OnUpdate() {
	std::string currentScene = SceneManager::GetInstance().GetActiveScene().name;
	bool isLevel = GameManager::GetInstance().IsLevelScene(currentScene);

	// no HUD on non-level scenes
	if (!isLevel) return;

	EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

	// Get Player
	if (!entityMgr.ValidateEntity(player_)) {
		std::vector<Entity> playerEntities = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);

		if (!playerEntities.empty()) {
			player_ = playerEntities[0];
		}
		else return; // return if can't find player
	}

	// Get Self
	if (!entityMgr.ValidateEntity(self_) || lifeNum_ <= 0) {
		self_ = GetEntity();
		if (entityMgr.HasComponent(self_, PlayerHealthComponentType)) {
			PlayerHealthComponent healthComp = entityMgr.GetComponent<PlayerHealthComponent>(self_);
			lifeNum_ = healthComp.lifeNum;
		}
	}

	// Check PlayerState.currentLives
	if (entityMgr.HasComponent(player_, PlayerStateType)) {
		PlayerState playerState = entityMgr.GetComponent<PlayerState>(player_);
		int playerLivesLeft = playerState.currentLives;

		// Check if this life is just lost
		if (!lifeLost_ && playerLivesLeft < lifeNum_) {
			if (entityMgr.HasComponent(self_, TextureComponentType)) {
				TextureComponent& textureComp = entityMgr.GetComponent<TextureComponent>(self_);
				textureComp.textureKey = "SB_HUD_EmptyHeart.png";
				lifeLost_ = true;
			}
		}
	}
}

void PlayerHealthHUD::OnDestroy() {
}
