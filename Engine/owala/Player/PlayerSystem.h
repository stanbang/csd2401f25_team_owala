 /*****************************************************************//**
 * \file   PlayerSystem.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief
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
		// CHANGED: Constructor now requires an InputSystem reference.
		PlayerSystem(EntityStorage& entityStorage, InputSystem& input, PhysicsSystem& physics)
			: entityStorage_(&entityStorage), input_(&input), physics_(&physics) {}
		void Initialize() override {}
		void Update(float dt) override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		WindowsSystem* win_ = nullptr;
		InputSystem* input_ = nullptr; // use our own input system
		PhysicsSystem* physics_ = nullptr; // use physics sytems
	};
} // namespace Framework
