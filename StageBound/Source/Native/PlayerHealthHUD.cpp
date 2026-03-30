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
			if (entityMgr.HasComponent(self_, TextureComponentType) && entityMgr.HasComponent(self_, TransformType)) {
				
				// Emit Poof VFX
				Vector2D position = entityMgr.GetComponent<Transform>(self_).position;
				TextureComponent& textureComp = entityMgr.GetComponent<TextureComponent>(self_);
				ParticleBurstEvent poofParticleEvent;

				// Convert position to (0 - 1) screen space for VFXSystem
				poofParticleEvent.position = {
					position.x * 0.5f + 0.5f,
					position.y * 0.5f + 0.5f
				};
				poofParticleEvent.isAnimated = true;
				poofParticleEvent.isScreenSpace = true;
				poofParticleEvent.textureKey = "SB_HeartPoof_Spritesheet.png";
				poofParticleEvent.size = {
					textureComp.HalfSize.x * 0.7f,
					textureComp.HalfSize.y * 0.8f,
				};
				poofParticleEvent.speed = 0.0;

				// Skip WorldToScreenSpace convertion on VFXSystem side
				poofParticleEvent.isUIVFX = true;
				poofParticleEvent.customRenderOrder = 199; // just below curtains

				// Render order:
				// Curtains - 200, PoofVFX - 199, Hearts - 198, HUDBoard - 197

				MessageBus::Instance().Publish(poofParticleEvent);

				// Swap texture to empty heart
				textureComp.textureKey = "SB_HUD_EmptyHeart.png";

				lifeLost_ = true;
			}
		}
	}
}

void PlayerHealthHUD::OnDestroy() {
}
