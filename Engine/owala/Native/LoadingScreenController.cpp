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

namespace Framework {

    REGISTER_SCRIPT(LoadingScreenController, "LoadingScreenController")

    void LoadingScreenController::OnCreate() {
		// Hide curtains immediately on loading screen creation
        TransitionController::ForceHideAll();
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
        if (rsm_.IsLoadingComplete() && !loadFadingOut && !loadingDone) {
            loadNextScene = GameManager::GetInstance().GetNextScene();
            Logger::Get().Info(DebugType::GameManager) << "Loading complete. Fading out before transition to: " << loadNextScene << std::endl;
            loadFadingOut = true;
        }

        // Tick fade
        if (loadFadingOut) {
            loadFadeAlpha += Framework::Time::DeltaTime() * 2.0f; // 0.5s, tune multiplier
            if (loadFadeAlpha >= 1.0f) {
                loadFadeAlpha = 1.0f;
                loadFadingOut = false;
                loadingDone = true;

                if (!loadNextScene.empty()) {
                    bool isTargetLevel = GameManager::GetInstance().IsLevelScene(loadNextScene);
                    TransitionController::s_startNextSceneClosed = isTargetLevel;
                    SceneManager::GetInstance().LoadScene(loadNextScene);
                    GameManager::GetInstance().SetNextScene("");
                    loadNextScene = "";
                }
                GameManager::GetInstance().SetOverlayCallback(nullptr);
                return;
            }
        }

        // Draw overlay
        if (loadFadeAlpha > 0.0f) {
            GameManager::GetInstance().SetOverlayCallback([]() {
                Graphics& gfx = *GameManager::GetInstance().GetSystems().gfx_;
                gfx.setAlphaBlend(true);
                gfx.drawRectangle({ -1.f, -1.f }, { 1.f, 1.f }, { 0.f, 0.f, 0.f, loadFadeAlpha });
                gfx.setAlphaBlend(false);
                });
        }
        else {
            GameManager::GetInstance().SetOverlayCallback(nullptr);
        }
    }

    void LoadingScreenController::OnDestroy() {
        // Cleanup if necessary
        loadFadeAlpha = 0.0f;
        loadFadingOut = false;
        loadingDone = false;
        loadNextScene = "";
        GameManager::GetInstance().SetOverlayCallback(nullptr);
    }
}