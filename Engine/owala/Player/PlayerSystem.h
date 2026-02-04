 /*****************************************************************//**
 * \file   PlayerSystem.h
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (50%)
 *		Sitara Anil
 * \brief	Contains declaration and definitions of a Player System that
 *			updates the player entity's components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include <Transformation.h>

namespace Framework {
	class TransformationSystem;
	class WindowsSystem; // Forward declare to get window handle for input
	class PlayerSystem : public ISystem {
	public:
	
		PlayerSystem() = default;
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

	private:
		// Player landed event handler
		void OnPlayerLanded(Entity player, const Vector2D& position);

		// Private variables
		//player death
		void CheckPlayerDeathFromTraps();
		void CheckPlayerDeathFromEnemies();
		void DamagePlayer(Entity playerEntity, const std::string& damageSource = "Unknown");
		void KillPlayer(Entity playerEntity, const std::string& killerType = "Unknown");
		bool IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius);
		void HandlePlayerDeath();
		void SeparatePlayerFromDynamicObjects(Entity playerEntity);
		
		//player movement
		float moveSpeed = 0.0f;
		float jumpHeight = 0.0f;

		//enemy death
		void CheckEnemyDeathFromTraps();
		void TriggerEnemyDeath(Entity enemyEntity, const std::string& killerType = "Unknown");

		static constexpr float INVULNERABILITY_DURATION = 1.0f;

		void UpdateInvulnerability();
	};
} // namespace Framework
