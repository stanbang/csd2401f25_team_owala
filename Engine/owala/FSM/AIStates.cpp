#include "AIStates.h"
#include "EntityManager.h"
#include "Components.h"
#include "FSMFactory.h" // Needed for Macros
#include <cmath>
#include "Vector2D.h"
#include "GameManager.h" // Required for HitWall condition
#include "PhysicsSystem.h"

namespace Framework {

    // --- HELPER ---
    static Entity GetPlayer(EntityStorage& ecs) {
        EntityManager mgr(ecs);
        auto players = mgr.GetAllEntitiesWithComponent(PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    // --- IDLE ---
    void BirdIdleState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }
    void BirdIdleState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        // Just waiting for transition (handled by FSM::Update)
    }
    void BirdIdleState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpRadius") wakeUpRadius_ = value;
    }

    // --- WAKING ---
    void BirdWakingState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
    }
    void BirdWakingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
    }
    void BirdWakingState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpTime") wakeUpTime_ = value;
    }

    // --- SIMPLE CHASE ---
    void BirdSimpleChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        // Just started chasing - no special setup needed
    }

    void BirdSimpleChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);

        if (player == 0 || !mgr.HasComponent(entity, TransformType) ||
            !mgr.HasComponent(player, TransformType)) return;

        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
        Vector2D targetPos = mgr.GetComponent<Transform>(player).position;

        // Simple: just fly straight toward player
        Vector2D direction = targetPos - myPos;
        direction.Normalize();

        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = direction * moveSpeed_;
        }
    }

    void BirdSimpleChaseState::SetParameter(const std::string& key, float value) {
        if (key == "moveSpeed") moveSpeed_ = value;
        if (key == "bounceForce") bounceForce_ = value;
        if (key == "verticalBias") verticalBias_ = value;
    }

    // --- CHASE ---
    void BirdChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        repathTimer_ = repathInterval_; // Force path update immediately
    }
    void BirdChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        repathTimer_ += dt;
        EntityManager mgr(ecs);

        Entity player = GetPlayer(ecs);
        if (player == 0 || !mgr.HasComponent(entity, TransformType) || !mgr.HasComponent(player, TransformType)) return;

        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
        Vector2D targetPos = mgr.GetComponent<Transform>(player).position;

        // 1. Recalculate Path periodically
        if (repathTimer_ >= repathInterval_ && navGrid_) {
            repathTimer_ = 0.0f;
            AStarPather pather;
            currentPath_ = pather.FindPath(*navGrid_, myPos, targetPos);
        }

        // 2. Follow Path
        if (!currentPath_.empty()) {
            Vector2D nextPoint = currentPath_.front();
            Vector2D dir = nextPoint - myPos;
            if (dir.Magnitude() < 10.0f) {
                currentPath_.erase(currentPath_.begin()); // Reached node
            }
            else {
                dir.Normalize();
                if (mgr.HasComponent(entity, VelocityType)) {
                    mgr.GetComponent<Velocity>(entity).Vel = dir * moveSpeed_;
                }
            }
        }
        else {
            // Fallback: move directly to player
            Vector2D dir = targetPos - myPos;
            dir.Normalize();
            if (mgr.HasComponent(entity, VelocityType)) {
                mgr.GetComponent<Velocity>(entity).Vel = dir * moveSpeed_;
            }
        }
    }
    void BirdChaseState::SetParameter(const std::string& key, float value) {
        if (key == "moveSpeed") moveSpeed_ = value;
    }

    // --- BOUNCE ---
    void BirdBounceState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, VelocityType)) return;

        // Get current velocity to determine bounce direction
        Vector2D currentVel = mgr.GetComponent<Velocity>(entity).Vel;

        // Bounce upward and slightly away from wall
        // Favor upward movement (like Demon Eye)
        bounceDirection_ = Vector2D(-currentVel.x * 0.5f, std::abs(currentVel.y) + 100.0f);
        bounceDirection_.Normalize();

        mgr.GetComponent<Velocity>(entity).Vel = bounceDirection_ * 250.0f;
    }

    void BirdBounceState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;

        // Gradually reduce bounce velocity
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;
            vel = vel * 0.95f; // Damping
        }
    }

    void BirdBounceState::SetParameter(const std::string& key, float value) {
        if (key == "duration") bounceDuration_ = value;
    }

    // --- HOVER ---
    void BirdHoverState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        attackTimer_ -= dt;
        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;

        // Simple orbiting/hover logic
        Vector2D dir = myPos - playerPos; // Vector away from player
        if (dir.Magnitude() < 0.1f) dir = { 1, 0 };
        dir.Normalize();

        Vector2D targetHoverPos = playerPos + dir * hoverRadius_;
        Vector2D moveDir = targetHoverPos - myPos;
        moveDir.Normalize();

        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = moveDir * hoverSpeed_;
        }
    }
    void BirdHoverState::SetParameter(const std::string& key, float value) {
        if (key == "hoverRadius") hoverRadius_ = value;
        if (key == "hoverSpeed") hoverSpeed_ = value;
        if (key == "attackCooldown") attackCooldown_ = value;
    }

    // --- ATTACK ---
    void BirdAttackState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;

        // Snapshot direction once
        direction_ = playerPos - myPos;
        direction_.Normalize();
    }
    void BirdAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = direction_ * attackSpeed_;
        }
    }
    void BirdAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackSpeed") attackSpeed_ = value;
        if (key == "duration") duration_ = value;
    }

    // --- RECOVERY ---
    void BirdRecoveryState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            // Stop moving, maybe bounce back slightly
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }
    void BirdRecoveryState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
    }
    void BirdRecoveryState::SetParameter(const std::string& key, float value) {
        if (key == "duration") duration_ = value;
    }

    // =========================================================
    // WITCH ENEMY STATE IMPLEMENTATIONS
    // =========================================================

    // --- WITCH IDLE ---
    void WitchIdleState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchIdleState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        // Just waiting for player detection (handled by FSM transitions)
    }

    void WitchIdleState::SetParameter(const std::string& key, float value) {
        if (key == "detectionRadius") detectionRadius_ = value;
    }

    // --- WITCH ALERTED ---
    void WitchAlertedState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        // Stop movement during alert animation
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchAlertedState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        // Play alert animation here if you have animation system
    }

    void WitchAlertedState::SetParameter(const std::string& key, float value) {
        if (key == "alertDuration") alertDuration_ = value;
    }

    // --- WITCH ENRAGED ---
    void WitchEnragedState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        // Stop movement during enrage animation
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchEnragedState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        // Play enraged animation here
    }

    void WitchEnragedState::SetParameter(const std::string& key, float value) {
        if (key == "enrageDuration") enrageDuration_ = value;
    }

    // --- WITCH TELEPORT ---
    void WitchTeleportState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        Entity player = GetPlayer(ecs);
        if (player == 0 || !mgr.HasComponent(entity, TransformType) ||
            !mgr.HasComponent(player, TransformType)) return;

        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;

        // Calculate teleport position near player
        Vector2D dirToPlayer = playerPos - myPos;
        dirToPlayer.Normalize();

        // Teleport to a position near the player (at teleportDistance_)
        targetPosition_ = playerPos - dirToPlayer * teleportDistance_;

        // Stop velocity during teleport
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchTeleportState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);

        // Teleport effect: move towards target over duration
        if (mgr.HasComponent(entity, TransformType)) {
            Vector2D& pos = mgr.GetComponent<Transform>(entity).position;

            // Lerp to target position
            float t = timer_ / teleportDuration_;
            if (t > 1.0f) t = 1.0f;

            Vector2D startPos = pos;
            pos = startPos + (targetPosition_ - startPos) * t;
        }
    }

    void WitchTeleportState::SetParameter(const std::string& key, float value) {
        if (key == "teleportDuration") teleportDuration_ = value;
        if (key == "teleportDistance") teleportDistance_ = value;
    }

    // --- WITCH CHARGING ---
    void WitchChargingState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        // Stop movement during charge
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchChargingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        // Show charging animation/effect
        // You can visualize the attack radius here in debug mode
    }

    void WitchChargingState::SetParameter(const std::string& key, float value) {
        if (key == "chargeDuration") chargeDuration_ = value;
        if (key == "attackRadius") attackRadius_ = value;
    }

    // --- WITCH ATTACK ---
    void WitchAttackState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        damageDealt_ = false;

        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);

        // Deal damage once during the attack
        if (!damageDealt_ && timer_ >= attackDuration_ * 0.3f) {
            damageDealt_ = true;

            // Check if player is in attack radius
            Entity player = GetPlayer(ecs);
            if (player != 0 && mgr.HasComponent(entity, TransformType) &&
                mgr.HasComponent(player, TransformType)) {

                Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
                Vector2D playerPos = mgr.GetComponent<Transform>(player).position;

                float distance = myPos.Distance(playerPos);

                if (distance <= attackRadius_) {
                    // Deal damage to player
                    // If you have a Health component, reduce it here
                    // Example: mgr.GetComponent<Health>(player).TakeDamage(10);
                }
            }
        }
    }

    void WitchAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackDuration") attackDuration_ = value;
        if (key == "attackRadius") attackRadius_ = value;
    }


    // =========================================================
    // MACRO REGISTRATION 
    // =========================================================
    REGISTER_STATE("Idle", BirdIdleState);
    REGISTER_STATE("Waking", BirdWakingState);
    REGISTER_STATE("Chase", BirdChaseState);
    REGISTER_STATE("Hover", BirdHoverState);
    REGISTER_STATE("Attack", BirdAttackState);
    REGISTER_STATE("Recovery", BirdRecoveryState);
    REGISTER_STATE("SimpleChase", BirdSimpleChaseState)
    REGISTER_STATE("Bounce", BirdBounceState)

    // witch registration 
    REGISTER_STATE("WitchIdle", WitchIdleState);
    REGISTER_STATE("WitchAlerted", WitchAlertedState);
    REGISTER_STATE("WitchEnraged", WitchEnragedState);
    REGISTER_STATE("WitchTeleport", WitchTeleportState);
    REGISTER_STATE("WitchCharging", WitchChargingState);
    REGISTER_STATE("WitchAttack", WitchAttackState);

    // Common Condition Helpers
    REGISTER_CONDITION(PlayerInRange, [](const rapidjson::Value& data) {
        float r = data.HasMember("radius") ? data["radius"].GetFloat() : 300.0f;

        return [r](Entity e, EntityStorage& ecs) {
            EntityManager mgr(ecs);
            Entity p = GetPlayer(ecs);
            if (p == 0) return false;
            Vector2D posA = mgr.GetComponent<Transform>(e).position;
            Vector2D posB = mgr.GetComponent<Transform>(p).position;
            return posA.Distance(posB) < r;
        };
    });

    REGISTER_CONDITION(TimerExpired, [](const rapidjson::Value& data) {
        std::string sName = data["state"].GetString();
        return [sName](Entity e, EntityStorage& ecs) {
            EntityManager mgr(ecs);
            FSMComponent& fsm = mgr.GetComponent<FSMComponent>(e);
            State* s = fsm.fsm.GetState(sName);

            // 1. Check Waking State
            if (auto* ws = dynamic_cast<BirdWakingState*>(s))
                return ws->GetTimer() >= ws->GetWakeUpTime();

            // 2. Check Bounce State
            if (auto* bs = dynamic_cast<BirdBounceState*>(s))
                return bs->GetTimer() >= bs->GetDuration();

            // 3. Check Attack State
            if (auto* as = dynamic_cast<BirdAttackState*>(s))
                return as->GetTimer() >= as->GetDuration();

            // 4. Check Recovery State
            if (auto* rs = dynamic_cast<BirdRecoveryState*>(s))
                return rs->GetTimer() >= rs->GetDuration();

            return false;
        };
    });


        // AIStates.cpp, improved collision detection
    REGISTER_CONDITION(HitWall, [](const rapidjson::Value& data) {
        return [](Entity e, EntityStorage& ecs) {
            EntityManager mgr(ecs);

            if (!mgr.HasComponent(e, VelocityType) ||
                !mgr.HasComponent(e, TransformType) ||
                !mgr.HasComponent(e, AABBColliderType)) {
                return false;
            }

            // Get physics system to check if we hit something
            auto* physics = GameManager::GetInstance().GetSystems().physics_;
            if (!physics) return false;

            // Get all platform entities
            auto platforms = mgr.GetAllEntitiesWithComponent(PlatformTagType);

            // Check if we're overlapping with any platform
            for (Entity platform : platforms) {
                if (physics->CheckTriggerOverlap(e, platform)) {
                    // We hit a wall/platform!
                    return true;
                }
            }

            return false;
            };
        });




}
