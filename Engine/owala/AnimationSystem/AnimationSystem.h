/*****************************************************************//**
 * \file   AnimationSystem.h
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
