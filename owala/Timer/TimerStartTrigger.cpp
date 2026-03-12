/*********************************************************************//**
 * \file   TimerStartTrigger.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Implementation of the TimerStartTriggerScript that detects when the
 *      player enters the level start zone and begins the gameplay timer.
 *
 * \details
 *      This script is attached to trigger zones placed at level start points.
 *      It continuously checks for collision between the player and the trigger,
 *      and starts the TimerSystem when contact occurs. The script automatically
 *      derives the level name from the active scene, eliminating the need for
 *      game-specific state flags.
 *
 *      Key responsibilities:
 *        - Detecting player-trigger collisions via AABB collision detection
 *        - Starting the TimerSystem when the player enters the level
 *        - Deriving level name from the current scene
 *        - Preventing multiple firings of the same trigger
 *        - Logging debug information about timer start
 *
 * \note
 *      This script uses the scene name as the level identifier for the timer,
 *      which is then used by TimerSystem for Firebase submissions when the
 *      level is completed.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "TimerStartTrigger.h"
#include "ScriptRegistry.h"
#include "TimerSystem.h"
#include "AABB_Collision.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "SceneManager.h"
#include "Logger.h"

REGISTER_SCRIPT(TimerStartTriggerScript, "TimerStartTriggerScript")

// -----------------------------------------------------------------------

/**
 * @brief Initializes the trigger script when the entity is created.
 *
 * Sets the initial fired state to false, ensuring the trigger is ready
 * to detect player entry.
 */
    void TimerStartTriggerScript::OnCreate() {
    m_fired = false;
}

// -----------------------------------------------------------------------

/**
 * @brief Updates the trigger check every frame.
 *
 * Performs the following checks in order:
 * 1. If trigger already fired, early return (prevents multiple triggers)
 * 2. Retrieve player and trigger entities from ECS
 * 3. Verify all required components exist (AABBCollider, Transform)
 * 4. Calculate world-space AABB bounds for both entities
 * 5. Test for collision using AABB intersection
 * 6. If collision detected, get scene name and start timer
 *
 * @note Only the first player entity found in the ECS is checked.
 * @warning Requires both player and trigger to have AABBCollider and Transform
 *          components, otherwise collision check is skipped.
 */
void TimerStartTriggerScript::OnUpdate() {
    if (m_fired) return;

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
        // Derive level name from the active scene - no game-specific flags needed.
        std::string sceneName = SceneManager::GetInstance().GetActiveScene().name;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerStart] Starting timer for scene: " << sceneName << std::endl;

        TimerSystem::Get().StartTimer(sceneName);
        m_fired = true;
    }
}

// -----------------------------------------------------------------------

/**
 * @brief Cleans up when the trigger entity is destroyed.
 *
 * Empty implementation as no cleanup is needed for this trigger.
 */
void TimerStartTriggerScript::OnDestroy() {}