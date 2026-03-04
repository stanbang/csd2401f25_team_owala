/*****************************************************************//**
 * \file   TextFadeScript.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief	Source file to implement a fade in-out effect for world text
 *			depending on distance between player and text.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "TextFadeScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(TextFadeScript, "TextFadeScript")

void TextFadeScript::OnCreate() {
}

void TextFadeScript::OnUpdate() {
	EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

	// Get Player
	std::vector<Entity> playerEntities = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
	Entity player = 0;

	if (!playerEntities.empty()) {
		player = playerEntities[0];
	}
	else return;

	// Get Self
	Entity self = GetEntity();

	// Calculate distance between player to self
	float distance = 0.0f;
	if (entityMgr.HasComponent(player, TransformType) && entityMgr.HasComponent(self, TransformType)) {
		Transform playerTransform = entityMgr.GetComponent<Transform>(player);
		Transform selfTransform = entityMgr.GetComponent<Transform>(self);
		
		distance = (selfTransform.position - playerTransform.position).Magnitude();

	}
	else return;

	// Set Text Opacity
	if (entityMgr.HasComponent(self, ComponentType::TextComponentType)) {
		TextComponent& text = entityMgr.GetComponent<TextComponent>(self);

		float ratio = distance / MAX_DISTANCE;
		float newAlpha = 1.0f - ratio;
		text.color.a = std::clamp(newAlpha, 0.0f, 1.0f);
	}
}

void TextFadeScript::OnDestroy() {
	// Logic to be called before Entity is deleted
}
