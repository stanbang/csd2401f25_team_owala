// include/Framework/GameSystem.h
#pragma once
#include "ISystem.h"
namespace Framework { class CoreEngine; class Graphics; }

namespace Framework {
	class GameSystem : public ISystem {
	public:
		GameSystem(CoreEngine& core, Graphics& gfx) : core_(&core), gfx_(&gfx) {}
		void Initialize() override {}
		void Update(float dt) override;
		void Shutdown() override {}
	private:
		CoreEngine* core_ = nullptr;
		Graphics* gfx_ = nullptr;
		float t_ = 0.f;
	};
} // namespace Framework
