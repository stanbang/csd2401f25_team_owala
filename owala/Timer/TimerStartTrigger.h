/*********************************************************************//**
 * \file   TimerStartTrigger.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the TimerStartTriggerScript that detects when the
 *      player enters the level start zone and begins the gameplay timer.
 *
 * \details
 *      This script component should be attached to trigger zone entities
 *      placed at level start points. It monitors collisions with the player
 *      and triggers the timer start functionality when the player begins
 *      the level. The script automatically derives the level name from the
 *      active scene, eliminating the need for game-specific state flags.
 *
 *      The script ensures:
 *        - The trigger only fires once per level entry
 *        - Proper collision detection using AABB bounds
 *        - Automatic level name detection from scene system
 *
 *      Example Usage:
 *      \code
 *      // In level editor/loading:
 *      Entity startZone = ecs.CreateEntity();
 *      startZone.AddComponent<Transform>({0.0f, 0.0f, 0.0f});
 *      startZone.AddComponent<AABBCollider>({3.0f, 3.0f}); // Trigger bounds
 *
 *      // Attach this script
 *      auto& script = startZone.AddComponent<ScriptComponent>();
 *      script.Bind<TimerStartTriggerScript>();
 *      \endcode
 *
 * \note
 *      This script only starts the timer - it does not handle any other
 *      level initialization logic. The level name is automatically derived
 *      from the currently active scene.
 *
 * \warning
 *      The trigger entity must have both Transform and AABBCollider
 *      components for collision detection to work. The player entity must
 *      have a PlayerTag component for identification.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ScriptableEntity.h"

using namespace Framework;

/**
 * @brief Script that detects player entry into the level and starts the timer.
 *
 * This script is designed to be attached to trigger zones at level start points.
 * When the player enters the trigger zone, it starts the TimerSystem and
 * passes the current scene name as the level identifier. The trigger can only
 * fire once to prevent restarting the timer mid-level.
 *
 * Required components on the same entity:
 * - Transform: For world position
 * - AABBCollider: For collision detection bounds
 *
 * Required components on the player entity:
 * - Transform: For world position
 * - AABBCollider: For collision detection bounds
 * - PlayerTag: To identify the player entity
 */
class TimerStartTriggerScript : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the trigger when the entity is created.
     *
     * Sets the fired state to false, preparing the trigger for player detection.
     */
    void OnCreate() override;

    /**
     * @brief Updates the trigger check every frame.
     *
     * Performs collision detection between the player and this trigger.
     * If collision occurs, starts the timer with the current scene name.
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up when the trigger entity is destroyed.
     *
     * Empty implementation as no cleanup is required for this trigger.
     */
    void OnDestroy() override;

private:
    bool m_fired = false; //!< Prevents multiple firings of the same trigger
};