/*****************************************************************//**
 * \file   PauseMenuController.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the PauseMenuController script to handle
 *          the pause menu functionality.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "PauseMenuController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "InputSystem.h"
#include "SceneManager.h"

REGISTER_SCRIPT(PauseMenuController, "PauseMenuController")

void PauseMenuController::OnCreate() {
	// Initialization Code Here
}

void PauseMenuController::OnUpdate() {
    InputSystem& input = *GameManager::GetInstance().GetSystems().input_;

    // Check if the P key was just pressed
    if (input.KeyPressed(InputSystem::Key::P)) {
        const SceneManagement::SceneAsset& optionsScene = SceneManager::GetInstance().GetSceneByName("OptionsMenu", true);

        // Toggle scene based on whether it is currently loaded
        if (optionsScene.IsValid() && optionsScene.isLoaded) {
            // Close scene if already open
            SceneManager::GetInstance().UnloadScene("OptionsMenu");
        }
        else {
            // Load scene on top of current scene
            SceneManager::GetInstance().LoadScene("OptionsMenu", LoadSceneMode::Additive);
        }
    }
}

void PauseMenuController::OnDestroy() {
	// Logic to be called before Entity is deleted
}
