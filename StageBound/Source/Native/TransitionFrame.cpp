/******************************************************************
 * \file   TransitionFrame.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the TransitionFrame script to manage the static
 *          frame sprite visibility during curtain transitions.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "TransitionFrame.h"
#include "ScriptRegistry.h"
#include "Components.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"

namespace Framework {

    REGISTER_SCRIPT(TransitionFrame, "TransitionFrame")

    void TransitionFrame::OnCreate() {
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        // Store the original texture key if it exists
        if (entityMgr.HasComponent(e, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
            if (!tex.textureKey.empty()) {
                frameTextureKey = tex.textureKey;
            }
        }

        // Initialize visibility based on current scene immediately
        UpdateVisibility();
    }

    void TransitionFrame::OnUpdate() {
        UpdateVisibility();
    }

    void TransitionFrame::OnDestroy() {
        // Nothing special needed on destroy
    }

    void TransitionFrame::UpdateVisibility() {
        EntityManager entityMgr(*GameManager::GetInstance().GetSystems().ecs_);
        Entity e = GetEntity();

        if (!entityMgr.HasComponent(e, TextureComponentType)) return;

        TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);

        // Get current scene and check if it's a level
        std::string currentScene = SceneManager::GetInstance().GetActiveScene().name;
        bool isLevel = GameManager::GetInstance().IsLevelScene(currentScene);

        // Frame should be visible ONLY in level scenes
        // It should be hidden in ALL non-level scenes (MainMenu, LevelSelection, etc.)
        bool shouldBeVisible = isLevel;

        if (shouldBeVisible && !isVisible_) {
            // Show the frame
            if (!frameTextureKey.empty()) {
                tex.textureKey = frameTextureKey;
            }
            isVisible_ = true;
        }
        else if (!shouldBeVisible && isVisible_) {
            // Hide the frame
            tex.textureKey = "";
            isVisible_ = false;
        }
        else if (!shouldBeVisible && !isVisible_ && !tex.textureKey.empty()) {
            // Edge case: ensure texture is cleared even if isVisible_ is already false
            // This handles cases where the texture might have been set externally
            tex.textureKey = "";
        }
    }
}