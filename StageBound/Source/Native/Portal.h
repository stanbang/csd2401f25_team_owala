/*********************************************************************//**
 * \file   Portal.h
 *
 * \primary_author
 *      Jason Lou (100%)
 *
 * \brief  Declaration of the Portal script for level exit functionality.
 *         Handles player interaction, level unlocking, and timer submission.
 *
 * \details
 *      This script is attached to portal entities that serve as level exits.
 *      When the player stands in the portal trigger and holds the F key for
 *      1.5 seconds, the level is marked as complete. The portal manages:
 *        - Player detection and proximity tracking
 *        - Hold-to-interact timer (1.5 second requirement)
 *        - Visual feedback via fading portal text
 *        - VFX and SFX triggering during interaction
 *        - Level unlocking via SaveManager
 *        - Timer stop and Firebase submission
 *        - Publishing LevelClearEvent for other systems
 *
 *      Example Usage:
 *      \code
 *      // In level editor, create portal entity with:
 *      Entity portal = ecs.CreateEntity();
 *      portal.AddComponent<Transform>({exitX, exitY, 0.0f});
 *      portal.AddComponent<AABBCollider>({50.0f, 50.0f}); // Trigger zone
 *
 *      // Add child text entity for "Press F to exit"
 *      Entity portalText = ecs.CreateEntity();
 *      portalText.AddComponent<Transform>({exitX, exitY - 30.0f, 0.0f});
 *      portalText.AddComponent<TextComponent>();
 *      portalText.GetProperties().name = "PortalText";
 *
 *      // Attach this script to portal
 *      auto& script = portal.AddComponent<ScriptComponent>();
 *      script.Bind<Portal>();
 *      \endcode
 *
 * \note
 *      The portal expects a child entity named "PortalText" with a TextComponent
 *      for visual feedback. The SaveManager must exist in the DDOL scene.
 *
 * \warning
 *      The portal requires a trigger AABBCollider for overlap detection.
 *      The player entity must have a PlayerTag component.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"
#include "Entity.h"
#include "SaveManager.h"

using namespace Framework;

/**
 * @brief Manages level exit portal functionality with hold-to-interact mechanics.
 *
 * This script handles the complete level completion flow:
 * 1. Detects player proximity and fades in instruction text
 * 2. Requires player to hold F for 1.5 seconds to confirm
 * 3. Triggers VFX/SFX during hold
 * 4. Stops the gameplay timer and submits to Firebase
 * 5. Unlocks the next level via SaveManager
 * 6. Publishes LevelClearEvent for other systems to react
 */
class Portal : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the portal when the entity is created.
     *
     * Determines which level index to unlock based on the current scene name.
     */
    void OnCreate() override;

    /**
     * @brief Updates the portal each frame, handling player interaction.
     *
     * Manages player detection, hold timer, visual feedback, and level completion.
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up when the portal entity is destroyed.
     */
    void OnDestroy() override;

private:
    // -------- Entity References --------
    Entity player = 0;           //!< Cached reference to the player entity
    Entity portalText = 0;       //!< Cached reference to the portal's text entity
    SaveManager* save = nullptr; //!< Cached reference to SaveManager for level unlocking

    // -------- Configuration --------
    int levelIndexToUnlock = 2;  //!< Level index to unlock when portal is activated

    // -------- State Tracking --------
    float interactTimer = 0.f;   //!< Accumulator for hold-to-interact timer (1.5s target)
    bool hasStartedAmbientSound = false;

    // -------- Entity Search Methods --------
    /**
     * @brief Searches the current scene for the player entity.
     */
    void SearchForPlayer();

    /**
     * @brief Searches for the portal's child text entity (named "PortalText").
     */
    void SearchForPortalChildren();

    /**
     * @brief Searches for the SaveManager in the persistent DDOL scene.
     */
    void SearchForSaveManager();

    // -------- Visual Feedback --------
    /**
     * @brief Updates the portal text alpha based on player proximity.
     *
     * Fades text in as player gets closer to the portal.
     *
     * @param entityMgr Reference to the EntityManager for component access
     */
    void UpdateChildren(EntityManager& entityMgr);
};