// include/Framework/GameSystem.h
#pragma once
#include "ISystem.h"
#include "Entity.hpp"

namespace Framework { class CoreEngine; class Graphics; class PhysicsSystem;}

namespace Framework {
	class GameSystem : public ISystem {
	public:
		GameSystem(CoreEngine& core, Graphics& gfx, PhysicsSystem& physics) : core_(&core), gfx_(&gfx), physics_(&physics) {}
		void Initialize() override {}
		void Update(float dt) override;
		void Shutdown() override {}
	private:
		CoreEngine* core_ = nullptr;
		Graphics* gfx_ = nullptr;
		float t_ = 0.f;
		PhysicsSystem* physics_ = nullptr;

		Entity fallingBox{};
		Entity groundPlane{}; // New entity for the ground
	};
} // namespace Framework
