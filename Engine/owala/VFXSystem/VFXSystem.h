/*****************************************************************//**
 * \file   VFXSystem.h
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
#include "ResourceManager.h"

namespace Framework {
	class WindowsSystem; // Forward declare to get window handle for input
	class VFXSystem : public ISystem {
	public:
		// Constructor taking references to EntityStorage and ResourceManager
		VFXSystem(EntityStorage& entityStorage, ResourceManager& resourceManager)
			: entityStorage_(&entityStorage), resourceManager_(&resourceManager) {
		}
		void Initialize() override {}
		void Update() override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		ResourceManager* resourceManager_ = nullptr;
		WindowsSystem* win_ = nullptr;
	};
} // namespace Framework