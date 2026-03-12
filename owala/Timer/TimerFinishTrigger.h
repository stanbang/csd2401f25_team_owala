/*********************************************************************//**
 * \file   TimerFinishTrigger.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the TimerFinishTriggerScript that detects when the
 *      player reaches the level exit portal and stops the gameplay timer.
 *
 * \details
 *      This script component should be attached to trigger zone entities
 *      placed at level exit points (portals, doors, etc.). It monitors
 *      collisions with the player and triggers the timer stop functionality
 *      when the player reaches the exit.
 *
 *      The script ensures:
 *        - The trigger only fires once per level
 *        - The timer is only stopped when it's actively running
 *        - Proper collision detection using AABB bounds
 *
 *      Example Usage:
 *      \code
 *      // In level editor/loading:
 *      Entity exitPortal = ecs.CreateEntity();
 *      exitPortal.AddComponent<Transform>({10.0f, 5.0f, 0.0f});
 *      exitPortal.AddComponent<AABBCollider>({2.0f, 2.0f}); // Trigger bounds
 *
 *      // Attach this script
 *      auto& script = exitPortal.AddComponent<ScriptComponent>();
 *      script.Bind<TimerFinishTriggerScript>();
 *      \endcode
 *
 * \note
 *      This script only stops the timer - it does not handle Firebase
 *      submission or leaderboard display. Those are handled by a separate
 *      callback set in TimerSystem by game code.
 *
 * \warning
 *      The trigger entity must have both Transform and AABBCollider
 *      components for collision detection to work.
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
 * @brief Script that detects player-trigger collisions and stops the level timer.
 *
 * This script is designed to be attached to trigger zones at level exit points.
 * When the player enters the trigger zone and the timer is running, it stops
 * the timer and marks the level as completed. The trigger can only fire once
 * to prevent multiple stops.
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
class TimerFinishTriggerScript : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the trigger when the entity is created.
     *
     * Sets the fired state to false and logs creation.
     */
    void OnCreate() override;

    /**
     * @brief Updates the trigger check every frame.
     *
     * Performs collision detection between the player and this trigger.
     * If collision occurs and timer is running, stops the timer.
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up when the trigger entity is destroyed.
     *
     * Logs the destruction for debugging.
     */
    void OnDestroy() override;

private:
    bool m_fired = false; //!< Prevents multiple firings of the same trigger
};