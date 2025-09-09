// Graphics.h
#pragma once
#include "ISystem.h"
namespace Framework { class WindowsSystem; }

namespace Framework {
	class Graphics : public ISystem {
	public:
		explicit Graphics(WindowsSystem& win) : win_(&win) {}
		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override {}
	private:
		WindowsSystem* win_ = nullptr;
		float t_ = 0.f;
	};
} // namespace Framework
