/*****************************************************************//**
 * \file   PlayerSystem.hpp
 * \brief  
 * 
 * 
 * \author Ng Jing Wen
 * \date   10 September 2025
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
		void Shutdown() override {}

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		WindowsSystem* win_ = nullptr;
		InputSystem* input_ = nullptr; // use our own input system
		PhysicsSystem* physics_ = nullptr; // use physics sytems
	};
} // namespace Framework
