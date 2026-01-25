#include "OptionsMenuBackButton.h"
#include "ScriptRegistry.h"
#include "Components.h"
#include "SceneManager.h"

REGISTER_SCRIPT(OptionsMenuBackButton, "OptionsMenuBackButton")

void OptionsMenuBackButton::OnCreate() {
	// Initialization Code Here
}

void OptionsMenuBackButton::OnUpdate() {
    // Get self
    Entity self = GetEntity();
    EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);

    // Get the Button component attached to this entity
    if (entityMgr.HasComponent(self, ComponentType::ButtonType)) {
        const Button& btn = entityMgr.GetComponent<Button>(self);

        // When the button is clicked, unload the OptionsMenu
        if (btn.isHovered && !btn.isPressed && btn.wasHoveredLastFrame) {
            SceneManager::GetInstance().UnloadScene("OptionsMenu");
        }
    }
}

void OptionsMenuBackButton::OnDestroy() {
	// Logic to be called before Entity is deleted
}
