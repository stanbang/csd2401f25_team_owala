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
		
		PlayerSystem(EntityStorage& entityStorage, InputSystem& input, PhysicsSystem& physics, TransformationSystem& transformation)
			: entityStorage_(&entityStorage), input_(&input), physics_(&physics), transformationSystem_(&transformation) {}
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		WindowsSystem* win_ = nullptr;
		InputSystem* input_ = nullptr; // use our own input system
		PhysicsSystem* physics_ = nullptr; // use physics sytems
		TransformationSystem* transformationSystem_ = nullptr;

		//player death
		void CheckPlayerDeathFromTraps();
		void CheckPlayerDeathFromEnemies();
		void KillPlayer(Entity playerEntity, const std::string& killerType = "Unknown");
		bool IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius);
		//player movement
		float moveSpeed = 0.0f;
		float jumpHeight = 0.0f;
	};
} // namespace Framework
