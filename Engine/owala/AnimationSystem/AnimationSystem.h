/*****************************************************************//**
 * \file   AnimationSystem.hpp
 * \brief
 *
 *
 * \author Ng Jing Wen
 * \date   25 September 2025
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"

namespace Framework {
	class WindowsSystem; // Forward declare to get window handle for input
	class AnimationSystem : public ISystem {
	public:
		// CHANGED: Constructor now requires an InputSystem reference.
		AnimationSystem(EntityStorage& entityStorage)
			: entityStorage_(&entityStorage) {}
		void Initialize() override {}
		void Update(float dt) override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		WindowsSystem* win_ = nullptr;
	};
} // namespace Framework
