/*********************************************************************//**
 * \file   Portal.cpp
 *
 * \primary_author
 *      Jason Lou (100%)
 *
 * \brief  Implementation of the Portal script for level exit functionality.
 *         Handles player interaction, level unlocking, and timer submission.
 *
 * \details
 *      This file contains the implementation of the portal system that
 *      serves as level exits. When the player stands in the portal trigger
 *      and holds the F key for 1.5 seconds, the level is marked as complete.
 *      The script manages:
 *        - Player detection and proximity tracking
 *        - Hold-to-interact timer (1.5 second requirement)
 *        - Visual feedback via fading portal text
 *        - VFX and SFX triggering during interaction
 *        - Level unlocking via SaveManager
 *        - Timer stop and Firebase submission
 *        - Publishing LevelClearEvent for other systems
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "Portal.h"
#include "ScriptRegistry.h"

 // Dependencies
#include "Core.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "InputSystem.h"
#include "OwalaTime.h"
#include "Vector2D.h"
#include "TimerSystem.h"
#include "MessageBus.h"
#include <string>

REGISTER_SCRIPT(Portal, "Portal")

// -----------------------------------------------------------------------
// Entity Search Methods
// -----------------------------------------------------------------------

/**
 * @brief Searches the current scene for the player entity.
 *
 * Finds the first entity with a PlayerTag component in the active scene
 * and caches it for future use. Called when the cached player becomes invalid.
 */
    void Portal::SearchForPlayer() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);
    for (Entity entity : sceneEntities) {
        if (entityMgr.HasComponent(entity, PlayerTagType)) {
            player = entity;
            return;
        }
    }
}

/**
 * @brief Searches for the portal's child text entity.
 *
 * Finds an entity named "PortalText" in the current scene that has a
 * TextComponent. This entity provides visual feedback (e.g., "Press F to exit")
 * and fades based on player distance.
 */
void Portal::SearchForPortalChildren() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // Search for other entities tied to the portal
    std::vector<Entity> sceneEntities = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetInstance().GetActiveScene().name);
    for (Entity entity : sceneEntities) {
        if (!entityMgr.HasComponent(entity, TextComponentType)) continue;
        if (entityMgr.IsActive(entity) && (entityMgr.GetProperties(entity).name == "PortalText")) {
            portalText = entity;
            return;
        }
    }
}

/**
 * @brief Searches for the SaveManager in the persistent DDOL scene.
 *
 * Finds the SaveManager script instance in the Don't Destroy On Load scene
 * and caches it for level unlocking functionality.
 */
void Portal::SearchForSaveManager() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // Search for SaveManager
    const std::vector<Entity> persistents = SceneManager::GetInstance().GetEntitiesInScene(SceneManager::GetDDOLSceneID());
    for (Entity entity : persistents) {
        if (!entityMgr.ValidateEntity(entity)) continue;
        if ((save = entityMgr.GetScript<SaveManager>(entity))) {
            return;
        }
    }
}

// -----------------------------------------------------------------------
// Visual Feedback Methods
// -----------------------------------------------------------------------

/**
 * @brief Updates the portal text alpha based on player proximity.
 *
 * Fades in the portal text as the player gets closer:
 * - At distance >= 200 units: alpha = 0.0 (invisible)
 * - At distance 0: alpha = 1.0 (fully visible)
 * - Linear interpolation in between
 *
 * @param entityMgr Reference to the EntityManager for component access
 */
void Portal::UpdateChildren(EntityManager& entityMgr) {
    if (!entityMgr.HasComponent(player, TransformType) || !entityMgr.HasComponent(portalText, TransformType)
        || !entityMgr.HasComponent(portalText, TextComponentType)) return;

    Vector2D& playerPos = entityMgr.GetComponent<Transform>(player).position;
    Vector2D& portalPos = entityMgr.GetComponent<Transform>(GetEntity()).position;

    float distance = playerPos.Distance(portalPos);
    float maxDistance = 200.f;
    float alpha = 0.f;

    // If player is within 200 units, fade in based on distance
    if (distance < maxDistance) {
        alpha = 1.f - (distance / maxDistance);
    }

    TextComponent& text = entityMgr.GetComponent<TextComponent>(portalText);
    text.color.a = alpha;
}

// -----------------------------------------------------------------------
// Lifecycle Implementation
// -----------------------------------------------------------------------

/**
 * @brief Initializes the portal when the entity is created.
 *
 * Determines which level index to unlock based on the current scene name:
 * - "Tutorial 2" -> unlocks index 3
 * - "Level 1"    -> unlocks index 4
 * - Any other    -> unlocks index 2 (default)
 */
void Portal::OnCreate() {
    // Cache the levelIndex to unlock depending on the current level the portal is in.
    std::string sceneName = SceneManager::GetInstance().GetActiveScene().name;

    if (sceneName == "Tutorial 1") {
        levelIndexToUnlock = 2;
    }
    else if (sceneName == "Tutorial 2") {
        levelIndexToUnlock = 3;
    }
    else if (sceneName == "Level 1") {
        levelIndexToUnlock = 4;
    }
    else if (sceneName == "Level 2") {
        levelIndexToUnlock = 5;
    }
    else if (sceneName == "Level 3") {
        levelIndexToUnlock = 6;
    }
}

/**
 * @brief Updates the portal each frame, handling player interaction.
 *
 * Core functionality:
 * 1. Finds/caches required entities (player, portal text, save manager)
 * 2. Updates portal text alpha based on distance
 * 3. Checks if player is overlapping the portal trigger
 * 4. If overlapping and holding F:
 *    - Accumulates interact timer
 *    - Triggers VFX/SFX on first frame of interaction
 *    - When timer reaches 1.5s: stops timer, unlocks level, publishes events
 * 5. Resets timer and VFX flags if player releases F or leaves trigger
 */
void Portal::OnUpdate() {
    static bool playExitVFX = true;
    GameManager& gm = GameManager::GetInstance();
    PhysicsSystem& physicsSystem = *gm.GetSystems().physics_;
    AudioSystem& audioSystem = *gm.GetSystems().audio_;
    EntityManager entityMgr(*gm.GetSystems().ecs_);

    Entity self = GetEntity();

    if (!hasStartedAmbientSound && entityMgr.HasComponent(self, TransformType)) {
        Vector2D position = entityMgr.GetComponent<Transform>(self).position;

        MessageBus::Instance().Publish(PlaySound3DEvent{ "Portal_Idle", AudioType::SFX, 0.4f, true, true, position, 50.f, 400.f, self });
        std::cout << "Ambient Portal Playing" << std::endl;
        hasStartedAmbientSound = true;
    }

    if (player == 0 || !entityMgr.ValidateEntity(player)) SearchForPlayer();
    if (portalText == 0 || !entityMgr.ValidateEntity(portalText)) SearchForPortalChildren();
    if (!save || !entityMgr.ValidateEntity(save->GetEntity())) {
        SearchForSaveManager();
        return;
    }

    // Handle Portal Exit Text
    if (player != 0 && portalText != 0) {
        UpdateChildren(entityMgr);
    }

    if (player == 0) return; // No valid player found; can't check overlap

    InputSystem& input = *gm.GetSystems().input_;

    // Check for F6 Keypress - Cheat Code to skip to next level
    if (input.KeyPressed(InputSystem::Key::F6)) {
        Framework::Logger::Get().Info(DebugType::Game)
            << "[Portal] Cheat Code F6 pressed: Skipping to next level..." << std::endl;

        // Reset timer system
        if (TimerSystem::Get().IsRunning()) {
            TimerSystem::Get().Reset();
        }

        audioSystem.stopAll();
        save->Unlock(levelIndexToUnlock);
        MessageBus::Instance().Publish(LevelClearEvent{});
    }
    // Check for F7 Keypress - Cheat Code to skip to Portal
    else if (input.KeyPressed(InputSystem::Key::F7)) {
        Framework::Logger::Get().Info(DebugType::Game)
            << "[Portal] Cheat Code F7 pressed: Skipping to Portal..." << std::endl;
        
        Vector2D& playerPos = entityMgr.GetComponent<Transform>(player).position;
        Vector2D portalPos = entityMgr.GetComponent<Transform>(self).position;

        playerPos = portalPos;
    }

    // Check if player overlapping with portal
    if (physicsSystem.CheckTriggerOverlap(player, self)) {
        // Player must hold down for 1.5 seconds to trigger completion flag.
        if (input.KeyDown(InputSystem::Key::F)) {
            interactTimer += Time::DeltaTime();

            // If playExitVFX is true, it means we haven't played the exit VFX for this portal interaction yet, so we play it and set the flag to false to prevent replaying.
            if (playExitVFX) {
                playExitVFX = false; // Set to false to prevent replaying the VFX in subsequent frames of this interaction.

                // Get the portal's position for VFX spawning
                Vector2D position = entityMgr.GetComponent<Transform>(self).position;

                // Publish PortalExitVFX event for the portal exit VFX system to subscribe to and spawn VFX.
                MessageBus::Instance().Publish(VFXTriggerEvent{ "PortalExitVFX", true, position });
                MessageBus::Instance().Publish(PlaySoundEvent{ "Portal_Activate", AudioType::SFX, 0.4f, true, false, self });
            }

            if (interactTimer >= 1.5f) {
                interactTimer = 0.f;
                playExitVFX = true; // Reset for the next time the player interacts with the portal.

                // Stop the timer and submit to Firebase BEFORE resetting
                if (TimerSystem::Get().IsRunning()) {
                    Framework::Logger::Get().Info(DebugType::Game)
                        << "[Portal] Level complete! Stopping timer and submitting..." << std::endl;
                    TimerSystem::Get().Stop();
                }

                audioSystem.stopAll();
                save->Unlock(levelIndexToUnlock);
                MessageBus::Instance().Publish(LevelClearEvent{});
            }
        }
        else {
            playExitVFX = true; // Reset the VFX flag if player releases the key before completion.
            interactTimer = 0.f;
        }
    }
    else {
        playExitVFX = true; // Reset the VFX flag if player steps out of the portal trigger.
        interactTimer = 0.f;
    }
}

/**
 * @brief Cleans up when the portal entity is destroyed.
 *
 * Empty implementation as no cleanup is required.
 */
void Portal::OnDestroy() {
    Entity self = GetEntity();
    GameManager::GetInstance().GetSystems().audio_->StopAudio(self);
}