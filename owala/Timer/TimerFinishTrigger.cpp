/*********************************************************************//**
 * \file   TimerFinishTrigger.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Implementation of the TimerFinishTriggerScript that detects when the
 *      player reaches the level exit portal and stops the gameplay timer.
 *
 * \details
 *      This script is attached to trigger zones placed at level exit points
 *      (portals, doors, etc.). It continuously checks for collision between
 *      the player and the trigger, and stops the TimerSystem when contact
 *      occurs. The script ensures the trigger only fires once and only when
 *      the timer is actively running.
 *
 *      Key responsibilities:
 *        - Detecting player-trigger collisions via AABB collision detection
 *        - Stopping the TimerSystem when the player reaches the exit
 *        - Preventing multiple firings of the same trigger
 *        - Logging debug information about timer completion
 *
 * \note
 *      The actual Firebase submission and leaderboard display are handled
 *      by a separate callback set in TimerSystem by game code, not by this
 *      script.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "TimerFinishTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(TimerFinishTriggerScript, "TimerFinishTriggerScript")

// -----------------------------------------------------------------------

/**
 * @brief Initializes the trigger script when the entity is created.
 *
 * Sets the initial fired state to false and logs creation for debugging.
 * This ensures the trigger is ready to detect player collisions.
 */
    void TimerFinishTriggerScript::OnCreate() {
    m_fired = false;
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[TimerFinish] Created" << std::endl;
}

// -----------------------------------------------------------------------

/**
 * @brief Updates the trigger check every frame.
 *
 * Performs the following checks in order:
 * 1. If trigger already fired, early return (prevents multiple triggers)
 * 2. If timer not running, early return (ignore collisions when timer stopped)
 * 3. Retrieve player and trigger entities from ECS
 * 4. Verify all required components exist (AABBCollider, Transform)
 * 5. Calculate world-space AABB bounds for both entities
 * 6. Test for collision using AABB intersection
 * 7. If collision detected, stop timer and mark as fired
 *
 * @note Only the first player entity found in the ECS is checked.
 * @warning Requires both player and trigger to have AABBCollider and Transform
 *          components, otherwise collision check is skipped.
 */
void TimerFinishTriggerScript::OnUpdate() {
    if (m_fired)                          return;
    if (!TimerSystem::Get().IsRunning())  return;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager  mgr(ecs);
    Entity self = GetEntity();

    std::vector<Entity> players = mgr.GetAllEntitiesWithComponent(false, PlayerTagType);
    if (players.empty()) return;
    Entity player = players[0];

    if (!mgr.HasComponent(player, AABBColliderType)) return;
    if (!mgr.HasComponent(self, AABBColliderType)) return;
    if (!mgr.HasComponent(player, TransformType))    return;
    if (!mgr.HasComponent(self, TransformType))    return;

    AABBCollider& playerCol = mgr.GetComponent<AABBCollider>(player);
    AABBCollider& triggerCol = mgr.GetComponent<AABBCollider>(self);
    Transform& playerTf = mgr.GetComponent<Transform>(player);
    Transform& selfTf = mgr.GetComponent<Transform>(self);

    // Calculate world-space bounds for player AABB
    AABB_Bounds playerBounds{
        { playerTf.position.x - playerCol.HalfSize.x + playerCol.Offset.x,
          playerTf.position.y - playerCol.HalfSize.y + playerCol.Offset.y },
        { playerTf.position.x + playerCol.HalfSize.x + playerCol.Offset.x,
          playerTf.position.y + playerCol.HalfSize.y + playerCol.Offset.y }
    };

    // Calculate world-space bounds for trigger AABB
    AABB_Bounds triggerBounds{
        { selfTf.position.x - triggerCol.HalfSize.x + triggerCol.Offset.x,
          selfTf.position.y - triggerCol.HalfSize.y + triggerCol.Offset.y },
        { selfTf.position.x + triggerCol.HalfSize.x + triggerCol.Offset.x,
          selfTf.position.y + triggerCol.HalfSize.y + triggerCol.Offset.y }
    };

    if (CollisionIntersection_RectRect_Static(playerBounds, triggerBounds)) {
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerFinish] Portal reached! Elapsed: "
            << TimerSystem::Get().GetElapsed() << "s. Stopping timer." << std::endl;

        // Stop the timer. The OnRunFinished callback (set by game code) handles
        // Firebase submission, leaderboard display, etc.
        TimerSystem::Get().Stop();
        m_fired = true;
    }
}

// -----------------------------------------------------------------------

/**
 * @brief Cleans up when the trigger entity is destroyed.
 *
 * Logs the destruction for debugging purposes.
 */
void TimerFinishTriggerScript::OnDestroy() {
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[TimerFinish] Destroyed" << std::endl;
}