#include "FPS.h"
#include "ScriptRegistry.h"
#include "GameManager.h"

REGISTER_SCRIPT(FPS, "FPS")

void FPS::OnCreate() {
	accum_ = 0;
	frames_ = 0;
}

void FPS::OnUpdate() {
	EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
	InputSystem& inputSystem = *GameManager::GetInstance().GetSystems().input_;

	Entity self = GetEntity();

	if (inputSystem.KeyPressed(InputSystem::Key::F1)) {
		shouldHide = !shouldHide;
		std::cout << (shouldHide ? "Hidden FPS Counter!" : "Showing FPS Counter") << std::endl;
	}

	if (entityMgr.HasComponent(self, ComponentType::TextComponentType)) {
		accum_ += Framework::Time::UnscaledDeltaTime();
		++frames_;

		const float updateInterval = 0.25f;
		if (accum_ >= updateInterval) {
			float fps = frames_ / accum_;

			TextComponent& text = entityMgr.GetComponent<TextComponent>(self);
			text.isDirty = true;
			text.textToDraw = shouldHide ? "" : "FPS: " + std::to_string(static_cast<int>(fps+ 0.5f));
			accum_ = 0.f;
			frames_ = 0;
		}
	}
}

void FPS::OnDestroy() {
	// Logic to be called before Entity is deleted
}
