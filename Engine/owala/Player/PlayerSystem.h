 /*****************************************************************//**
 * \file   PlayerSystem.h
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (50%)
 *
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

namespace Framework {
	class WindowsSystem; // Forward declare to get window handle for input
	class PlayerSystem : public ISystem {
	public:
		
		PlayerSystem(EntityStorage& entityStorage, InputSystem& input, PhysicsSystem& physics)
			: entityStorage_(&entityStorage), input_(&input), physics_(&physics) {}
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		WindowsSystem* win_ = nullptr;
		InputSystem* input_ = nullptr; // use our own input system
		PhysicsSystem* physics_ = nullptr; // use physics sytems

		// Player landed event handler
		void OnPlayerLanded(Entity player, const Vector2D& position, float impactSpeed);

		// Private variables
		float moveSpeed = 0.0f;
		float jumpHeight = 0.0f;
	};
} // namespace Framework
