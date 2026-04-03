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
#include "SceneManager.h"
#include <Transformation.h>

namespace Framework {
	class TransformationSystem;
	class WindowsSystem; // Forward declare to get window handle for input

	struct PlayerStats {
		float moveSpeed = 0.0f;
		float jumpHeight = 0.0f;
		float invulnerabilityDuration = 0.0f;
		float knockbackDuration = 0.0f;
		float knockbackForceX = 0.0f;
		float knockbackForceY = 0.0f;
	};

	class PlayerSystem : public ISystem {
	public:
	
		PlayerSystem() = default;
		/*********** Loads player tuning, registers gameplay events, and prepares runtime state. ***********/
		void Initialize() override;
		/*********** Processes player input, movement state, collisions, and combat checks each frame. ***********/
		void Update() override;
		/*********** Shuts down the player system. ***********/
		void Shutdown() override;

	private:
		// Player landed event handler
		/*********** Publishes landing audio and VFX when the player touches down after a fall. ***********/
		void OnPlayerLanded(Entity player, const Vector2D& position);

		// Private variables
		//player death
		/*********** Detects falling trap collisions that should damage or crush the player. ***********/
		void CheckPlayerDeathFromTraps();
		/*********** Detects enemy contact that should damage the player. ***********/
		void CheckPlayerDeathFromEnemies();
		/*********** Applies damage, knockback, invulnerability, and feedback to the player. ***********/
		void DamagePlayer(Entity playerEntity, const std::string& damageSource, const Vector2D& sourcePos);
		/*********** Puts the player into the death state and triggers death events. ***********/
		void KillPlayer(Entity playerEntity, const std::string& killerType = "Unknown");
		/*********** Returns whether the player lies within an enemy attack radius. ***********/
		bool IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius);
		/*********** Monitors the death animation and fires the completion event once. ***********/
		void HandlePlayerDeath();
		/*********** Gradually pushes the invulnerable player out of overlapping moving hazards. ***********/
		void SeparatePlayerFromDynamicObjects(Entity playerEntity);
		
		//enemy death
		/*********** Detects trap collisions that should kill enemies. ***********/
		void CheckEnemyDeathFromTraps();
		/*********** Switches an enemy FSM into its death state and logs the kill source. ***********/
		void TriggerEnemyDeath(Entity enemyEntity, const std::string& killerType = "Unknown");
		/*********** Advances invulnerability timers and restores normal collision once hurt state ends. ***********/
		void UpdateInvulnerability();

		PlayerStats m_stats;
		bool m_deathEventFired = false;
		bool m_lvlClearEventFired = false;
		std::unordered_set<SceneGUID> m_spawnedScenes; // Track scenes where player was already spawned
		bool m_cheatGodMode = false;
	};
} // namespace Framework
