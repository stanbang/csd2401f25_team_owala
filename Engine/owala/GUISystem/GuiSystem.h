#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "ResourceManager.h"
#include "EntityManager.h"
#include "InputSystem.h"

namespace Framework{
	class GuiSystem : public ISystem {
	public:
		// Constructor taking references to EntityStorage and ResourceManager
		GuiSystem(EntityStorage& entityStorage, ResourceManager& resourceManager, InputSystem& inputSystem, EntityManager& entityManager)
			: entityStorage_(&entityStorage), resourceManager_(&resourceManager), inputSystem_(&inputSystem), entityMgr(entityManager) {
		}
		void Initialize() override {}
		void Update(float dt) override;
		void Shutdown() override;

	private:
		// Pointer to the entity storage to access and modify entities and their components
		EntityStorage* entityStorage_ = nullptr;
		ResourceManager* resourceManager_ = nullptr;
		InputSystem* inputSystem_ = nullptr;
		EntityManager& entityMgr;
	};
}
