/*****************************************************************//**
 * \file   LoadingScreenController.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the LoadingScreenController script to manage loading
 *          and transitioning between scenes with a loading screen.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "LoadingScreenController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Components.h"
#include "EntityManager.h"
#include <string>
#include <iomanip>
#include <sstream>

namespace Framework {

    REGISTER_SCRIPT(LoadingScreenController, "LoadingScreenController")

    void LoadingScreenController::OnCreate() {
	// Empty by design
    }

    void LoadingScreenController::OnUpdate() {
        // Get Systems
        ResourceManager& rsm_ = *GameManager::GetInstance().GetSystems().resourceManager_;
        EntityStorage& ecs_ = *GameManager::GetInstance().GetSystems().ecs_;

        // Use EntityManager wrapper to access private storage data safely
        EntityManager entityMgr(ecs_);

        // Get Loading Progress
        float progress = rsm_.GetLoadingProgress();

        // Update Text Component
        Entity myEntity = GetEntity();

		// Check if entity has TextComponent
        if (entityMgr.HasComponent(myEntity, TextComponentType)) {

            // Use EntityManager to get the component reference
            TextComponent& textComp = entityMgr.GetComponent<TextComponent>(myEntity);

            std::stringstream ss;
            ss << "Loading... " << std::fixed << std::setprecision(0) << (progress * 100.0f) << "%";
            textComp.textToDraw = ss.str();
            textComp.isDirty = true;
        }

        // Transition when done
        if (rsm_.IsLoadingComplete()) {
            std::string nextScene = GameManager::GetInstance().GetNextScene();
			//Log when loading is complete
			Logger::Get().Info(DebugType::GameManager) << "Loading complete. Transitioning to next scene: " << nextScene << std::endl;

            if (!nextScene.empty()) {
                SceneManager::GetInstance().LoadScene(nextScene);
                GameManager::GetInstance().SetNextScene("");
            }
        }
    }

    void LoadingScreenController::OnDestroy() {
        // Cleanup if necessary
    }
}