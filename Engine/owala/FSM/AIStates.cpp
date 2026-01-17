/*****************************************************************//**
* \file   BirdAIStates.cpp
*
* \brief  Implementation of Bird AI states.
*********************************************************************/
#include "AIStates.h"
#include "EntityManager.h"
#include "OwalaTime.h"
#include "Logger.h"
#include <algorithm>

namespace Framework {

    // Helper to get player entity
    static Entity GetPlayer(EntityStorage& ecs) {
        EntityManager mgr(ecs);
        auto players = mgr.GetAllEntitiesWithComponent(PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    // ============================================================================
    // IDLE STATE
    // ============================================================================

    void BirdIdleState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
        }
        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] entered IDLE state" << std::endl;
    }

    void BirdIdleState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        // Just wait - transition will handle waking up
        (void)entity; (void)ecs; (void)dt;
    }

    // ============================================================================
    // WAKING STATE
    // ============================================================================

    void BirdWakingState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity; (void)ecs; // Silence warnings
        timer_ = 0.0f;
        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] entered WAKING state" << std::endl;
    }

    void BirdWakingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; (void)ecs; // Silence warnings
        timer_ += dt;
        // Transition will check if timer >= wakeUpTime
    }

    void BirdWakingState::OnExit(Entity entity, EntityStorage& ecs) {
        (void)entity; (void)ecs; // Silence warnings
        timer_ = 0.0f;
    }

    // ============================================================================
    // CHASE STATE
    // ============================================================================

    void BirdChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity; (void)ecs; // Silence warnings
        currentPath_.clear();
        pathRecalculateTimer_ = 0.0f;
        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] entered CHASE state" << std::endl;
    }

    void BirdChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, PositionType) ||
            !mgr.HasComponent(entity, VelocityType)) {
            return;
        }

        Vector2D birdPos = mgr.GetComponent<Position>(entity).Pos;
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;

        // Recalculate path periodically
        pathRecalculateTimer_ -= dt;
        if (pathRecalculateTimer_ <= 0.0f || currentPath_.empty()) {
            if (navGrid_) {
                currentPath_ = pather_.FindPath(*navGrid_, birdPos, playerPos);
                pathRecalculateTimer_ = pathRecalculateInterval_;

                if (currentPath_.empty()) {
                    Logger::Get().Info(DebugType::AISystem)
                        << "Bird [" << entity << "] failed to find path!" << std::endl;
                }
            }
        }

        // Follow path
        if (!currentPath_.empty()) {
            Vector2D targetPoint = currentPath_[0];
            Vector2D direction = (targetPoint - birdPos).Normalized();
            mgr.GetComponent<Velocity>(entity).Vel = direction * moveSpeed_;

            float reachThreshold = 10.0f; // Default fallback
            if (navGrid_) {
                reachThreshold = navGrid_->cellSize * 0.5f;
            }

            // Check if reached waypoint
            if (birdPos.Distance(targetPoint) < reachThreshold) {
                currentPath_.erase(currentPath_.begin());
            }

            //// Check if reached waypoint
            //if (birdPos.Distance(targetPoint) < navGrid_->cellSize * 0.5f) {
            //    currentPath_.erase(currentPath_.begin());
            //}
        }
        else {
            // No path, move directly to player
            Vector2D direction = (playerPos - birdPos).Normalized();
            mgr.GetComponent<Velocity>(entity).Vel = direction * moveSpeed_;
        }
    }

    void BirdChaseState::OnExit(Entity entity, EntityStorage& ecs) {
        (void)entity; (void)ecs; // Silence warnings
        currentPath_.clear();
    }

    // ============================================================================
    // HOVER STATE
    // ============================================================================

    void BirdHoverState::OnEnter(Entity entity, EntityStorage& ecs) {
        currentHoverNode_ = 0;
        attackTimer_ = attackCooldown_;
        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] entered HOVER state" << std::endl;
    }

    void BirdHoverState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, PositionType) ||
            !mgr.HasComponent(entity, VelocityType)) {
            return;
        }

        Vector2D birdPos = mgr.GetComponent<Position>(entity).Pos;
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;

        // Calculate hover box nodes
        float boxHalfWidth = hoverRadius_;
        float boxHalfHeight = hoverRadius_;

        Vector2D hoverNodes[4];
        hoverNodes[0] = { playerPos.x - boxHalfWidth, playerPos.y + hoverHeight_ };
        hoverNodes[1] = { playerPos.x + boxHalfWidth, playerPos.y + hoverHeight_ };
        hoverNodes[2] = { playerPos.x + boxHalfWidth, playerPos.y + hoverHeight_ + boxHalfHeight };
        hoverNodes[3] = { playerPos.x - boxHalfWidth, playerPos.y + hoverHeight_ + boxHalfHeight };

        Vector2D targetNode = hoverNodes[currentHoverNode_];

        // Move towards node
        Vector2D direction = (targetNode - birdPos).Normalized();
        mgr.GetComponent<Velocity>(entity).Vel = direction * hoverSpeed_;

        // Check if reached node
        if (birdPos.Distance(targetNode) < hoverNodeReachedDist_) {
            currentHoverNode_ = (currentHoverNode_ + 1) % 4;

            // Start attack timer if not started
            if (attackTimer_ <= 0.0f) {
                attackTimer_ = attackCooldown_;
            }
        }

        // Update attack timer
        if (attackTimer_ > 0.0f) {
            attackTimer_ -= dt;
        }
    }

    // ============================================================================
    // ATTACK STATE
    // ============================================================================

    void BirdAttackState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);

        Vector2D birdPos = Vector2D::Zero();
        if (mgr.HasComponent(entity, PositionType)) {
            birdPos = mgr.GetComponent<Position>(entity).Pos;
        }

        Entity player = GetPlayer(ecs);
        Vector2D targetPos = birdPos; // Default to self if no player

        if (player != 0 && mgr.HasComponent(player, PositionType)) {
            targetPos = mgr.GetComponent<Position>(player).Pos;
        }

        // [LOGIC CHANGE] Snapshot the direction ONCE.
        // This creates the "straight line" behavior.
        diveDirection_ = (targetPos - birdPos).Normalized();

        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] DIVING! Dir: " << diveDirection_.x << ", " << diveDirection_.y << std::endl;
    }

    void BirdAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)dt; // Unused
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, VelocityType)) return;

        // [LOGIC CHANGE] Apply velocity strictly in the snapshotted direction.
        // We rely on the PhysicsSystem to stop us if we hit a wall.
        mgr.GetComponent<Velocity>(entity).Vel = diveDirection_ * attackSpeed_;
    }

    void BirdAttackState::OnExit(Entity entity, EntityStorage& ecs) {
        /*timer_ = 0.0f;*/
    }

    void BirdRecoveryState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        timer_ = 0.0f;

        // Stop moving immediately
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
        }

        Logger::Get().Info(DebugType::AISystem)
            << "Bird [" << entity << "] hit wall! Recovering..." << std::endl;
    }

    void BirdRecoveryState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; (void)ecs;
        timer_ += dt;
    }

} // namespace Framework