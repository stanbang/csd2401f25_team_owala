#include "OptionsMenuBackButton.h"
#include "ScriptRegistry.h"
#include "Components.h"
#include "SceneManager.h"

REGISTER_SCRIPT(OptionsMenuBackButton, "OptionsMenuBackButton")

void OptionsMenuBackButton::OnCreate() {
    shouldCloseMenu_ = false;
}

void OptionsMenuBackButton::OnUpdate() {
    if (shouldCloseMenu_) {
        SceneManager::GetInstance().UnloadScene("OptionsMenu");
        shouldCloseMenu_ = false;
        return; // Don't process button this frame
    }

    // Get self
    Entity self = GetEntity();
    EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

    // Get the Button component attached to this entity
    if (entityMgr.HasComponent(self, ComponentType::ButtonType)) {
        const Button& btn = entityMgr.GetComponent<Button>(self);

        // When the button is clicked, defer unload to next frame
        if (btn.isHovered && !btn.isPressed && btn.wasHoveredLastFrame) {
            shouldCloseMenu_ = true;
        }
    }
}

void OptionsMenuBackButton::OnDestroy() {
	// Logic to be called before Entity is deleted
}
