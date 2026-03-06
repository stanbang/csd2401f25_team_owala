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
#include "pch.h"
#include "LoadingScreenController.h"
#include "TransitionController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Components.h"
#include "EntityManager.h"
#include "LevelSelection.h"

namespace Framework {

    REGISTER_SCRIPT(LoadingScreenController, "LoadingScreenController")

        void LoadingScreenController::OnCreate() {
        // Hide curtains immediately on loading screen creation
        TransitionController::ForceHideAll();
        loadingDone = false;
        loadNextScene = "";
        // The scene was loaded while the screen was black (GameManager faded out first).
        // Fade in now to reveal the loading screen.
        GameManager::GetInstance().FadeIn(0.5f);
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

        // COMMENT OUT OR DELETE THIS DIRECT TRANSITION SECTION
        /*
        // Transition when done - DIRECT, NO FADE
        if (rsm_.IsLoadingComplete() && !loadingDone) {
            std::string nextScene = GameManager::GetInstance().GetNextScene();
            Logger::Get().Info(DebugType::GameManager) << "Loading complete. Transitioning to: " << nextScene << std::endl;

            loadingDone = true;

            if (!nextScene.empty()) {
                bool isTargetLevel = GameManager::GetInstance().IsLevelScene(nextScene);
                TransitionController::s_startNextSceneClosed = isTargetLevel;
                SceneManager::GetInstance().LoadScene(nextScene);
                GameManager::GetInstance().SetNextScene("");
                loadNextScene = "";
            }
            GameManager::GetInstance().SetOverlayCallback(nullptr);
            return;
        }
        */

        // Wait for loading to finish and for the fade-in to complete, then fade out to the next scene.
        if (rsm_.IsLoadingComplete() && !loadingDone && !GameManager::GetInstance().IsFading()) {
            loadingDone = true;
            loadNextScene = GameManager::GetInstance().GetNextScene();
            Logger::Get().Info(DebugType::GameManager) << "Loading complete. Fading to: " << loadNextScene << std::endl;

            const std::string nextScene = loadNextScene;
            // Fade to black, then load the actual level (which starts with curtains closed
            // and will fade in on its own via s_startNextSceneClosed / WAITING state).
            GameManager::GetInstance().FadeOut(0.5f, [nextScene]() {
                TransitionController::s_startNextSceneClosed = GameManager::GetInstance().IsLevelScene(nextScene);
                SceneManager::GetInstance().LoadScene(nextScene);
                GameManager::GetInstance().SetNextScene("");
                // FadeIn is called automatically in the level's OnCreate or by the
                // curtain WAITING state completing - no manual FadeIn needed here.
                GameManager::GetInstance().FadeIn(0.5f);
                });
        }
    }

    void LoadingScreenController::OnDestroy() {
        // Cleanup
        loadingDone = false;
        loadNextScene = "";
        LevelSelection::ResetFade();
    }
}