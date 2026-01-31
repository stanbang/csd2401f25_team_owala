/*****************************************************************//**
 * \file   OptionsMenuBackButton.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the OptionsMenuBackButton script to handle
 *          the back button functionality in the options menu.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
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
        GameManager::GetInstance().GetSystems().physics_->SetPaused(false);
        SceneManager::GetInstance().UnloadScene("OptionsMenu");
        shouldCloseMenu_ = false;
        return; // Don't process button this frame
    }

    // Get self
    Entity self = GetEntity();
    EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
	InputSystem& inputSystem_ = *GameManager::GetInstance().GetSystems().input_;

    // Get the Button component attached to this entity
    if (entityMgr.HasComponent(self, ComponentType::ButtonType)) {
        const Button& btn = entityMgr.GetComponent<Button>(self);

        // When the button is clicked, defer unload to next frame
        if (btn.isHovered && inputSystem_.MouseReleased(InputSystem::MouseButton::Left)) {
            shouldCloseMenu_ = true;
        }
    }
}

void OptionsMenuBackButton::OnDestroy() {
	// Logic to be called before Entity is deleted
}
