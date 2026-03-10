/*********************************************************************//**
* \file   AIStates.cpp
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Implements the behavior logic for concrete AI states such as Idle,
* Waking, Chase, and Attack, including their transition triggers and
* parameter updates.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/

#include "pch.h"
#include "AIStates.h"
#include "EntityManager.h"
#include "Components.h"
#include "FSMFactory.h" 
#include "Vector2D.h"
#include "GameManager.h" 
#include "PhysicsSystem.h"

namespace Framework {
    
    // --- HELPER ---
    static Entity GetPlayer(EntityStorage& ecs) {
        EntityManager mgr(ecs);
        auto players = mgr.GetAllEntitiesWithComponent(true, PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    // Per-entity storage for the pre-collision velocity that SimpleChase
    // captured last frame.  Bounce reads it on enter.
    // (static map is fine; one entry per living bird enemy)
    static std::unordered_map<Entity, Vector2D> s_lastChaseVel;

    // Helper: clamp a float toward zero by a max magnitude
    static float ClampMagnitude(float value, float maxMag) {
        if (value > maxMag)  return maxMag;
        if (value < -maxMag) return -maxMag;
        return value;
    }

    // --- IDLE ---
    void BirdIdleState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }

        if (mgr.HasComponent(entity, TransformType)) {
            mgr.GetComponent<Transform>(entity).rotation.textureAngle = 0.0f;
        }

        s_lastChaseVel.erase(entity);

    }
    void BirdIdleState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)ecs;
        (void)dt;
        // Just waiting for transition 
        
    }
    void BirdIdleState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpRadius") wakeUpRadius_ = value;
        
    }

    // --- WAKING ---
    void BirdWakingState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity;
        (void)ecs;
        timer_ = 0.0f;

        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, TransformType)) {
            Vector2D position = mgr.GetComponent<Transform>(entity).position;
            MessageBus::Instance().Publish(BirdWakeEvent(entity, position));
        }
    }
    void BirdWakingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)ecs;
        timer_ += dt;
    }
    void BirdWakingState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpTime") wakeUpTime_ = value;
    }

    // --- SIMPLE CHASE ---
    void BirdSimpleChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        ownerID_ = entity;

        if (mgr.HasComponent(entity, VelocityType)) {
            s_lastChaseVel[entity] = mgr.GetComponent<Velocity>(entity).Vel;
        }
    }

    void BirdSimpleChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)dt;

        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);

        if (player == 0 || !mgr.HasComponent(entity, TransformType) || !mgr.HasComponent(player, TransformType)) {
            // Safety stop if player dies mid-chase
            return;
        }

        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
        Vector2D targetPos = mgr.GetComponent<Transform>(player).position;
        Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;

        float targetX = targetPos.x;
        float targetY = targetPos.y + hoverOffset_;

        // horizontal movement
        if (myPos.x < targetX) {
            // Target is to the right
            vel.x += accelX_ * dt;
            // Turnaround: if we're still moving LEFT, add extra kick
            if (vel.x < 0.0f) vel.x += accelX_ * turnaroundMult_ * dt;
        }
        else {
            // Target is to the left
            vel.x -= accelX_ * dt;
            if (vel.x > 0.0f) vel.x -= accelX_ * turnaroundMult_ * dt;
        }
        vel.x = ClampMagnitude(vel.x, maxSpeedX_);


        // ---------------------------------------------------------------
        // VERTICAL  accelerate toward targetY
        //  
        // ---------------------------------------------------------------
        if (myPos.y < targetY) {
            // We are BELOW the target - need to move UP
            vel.y += accelYUp_ * dt;
        }
        else {
            // We are ABOVE the target - swoop DOWN
            vel.y -= accelYDown_ * dt;
        }
        vel.y = ClampMagnitude(vel.y, maxSpeedY_);

        
        s_lastChaseVel[entity] = vel;

        if (mgr.HasComponent(entity, TransformType)) {
            Transform transform = mgr.GetComponent<Transform>(entity);
            transform.rotation.textureAngle = 0.0f;
        }
    }
    
    void BirdSimpleChaseState::SetParameter(const std::string& key, float value) {
        if (key == "accelX")         accelX_ = value;
        if (key == "accelYUp")       accelYUp_ = value;
        if (key == "accelYDown")     accelYDown_ = value;
        if (key == "maxSpeedX")      maxSpeedX_ = value;
        if (key == "maxSpeedY")      maxSpeedY_ = value;
        if (key == "hoverOffset")    hoverOffset_ = value;
        if (key == "turnaroundMult") turnaroundMult_ = value;
        // Legacy keys 
        if (key == "moveSpeed") {} //moveSpeed_ = value;
        if (key == "bounceForce") {} //bounceForce_ = value;
        if (key == "verticalBias") {}//verticalBias_ = value;
    }
    
    // --- CHASE ---
    void BirdChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity;
        (void)ecs;
        repathTimer_ = repathInterval_; // Force path update immediately
    }


    void BirdChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        repathTimer_ += dt;
        EntityManager mgr(ecs);

        Entity player = GetPlayer(ecs);
        if (player == 0 || !mgr.HasComponent(entity, TransformType) || !mgr.HasComponent(player, TransformType)) return;

        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
        Vector2D targetPos = mgr.GetComponent<Transform>(player).position;

        // Recalculate Path periodically
        if (repathTimer_ >= repathInterval_ && navGrid_) {
            repathTimer_ = 0.0f;
            AStarPather pather;
            currentPath_ = pather.FindPath(*navGrid_, myPos, targetPos);
        }

        // Follow Path
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
        ownerID_ = entity;
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, VelocityType)) return;

        // Get current velocity to determine bounce direction
        Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;

       
      
        Vector2D preVel = { 0.0f, 0.0f };
        auto it = s_lastChaseVel.find(entity);
        if (it != s_lastChaseVel.end()) {
            preVel = it->second;
        }

        // Determine which side we hit based on the cached X velocity
        // positive X  was moving right  hit a right wall  normal is -1
        collisionNormalX_ = (preVel.x >= 0.0f) ? -1.0f : 1.0f;

        
        // REFLECT X, retain bounceFactor_ of the speed
        
        vel.x = -preVel.x * bounceFactor_;
        
        if (std::abs(vel.y) < 0.1f) {
            vel.y = preVel.y;  // preserve Y momentum
        }

        
        if (mgr.HasComponent(entity, TransformType)) {
            Transform transform = mgr.GetComponent<Transform>(entity);
            transform.position.x += collisionNormalX_ * nudgeAmount_;
            // Reset texture rotation, Bounce uses the fly animation flat
            transform.rotation.textureAngle = 0.0f;
        }
    }

    void BirdBounceState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;

        // Gradually reduce bounce velocity
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            s_lastChaseVel[entity] = mgr.GetComponent<Velocity>(entity).Vel;
        }
    }

    void BirdBounceState::SetParameter(const std::string& key, float value) {
        if (key == "duration") bounceDuration_ = value;
        if (key == "bounceFactor")  bounceFactor_ = value;
        if (key == "nudgeAmount")   nudgeAmount_ = value;
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
        ownerID_ = entity;
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;

        // Snapshot direction once
        direction_ = playerPos - myPos;
        direction_.Normalize();

        // upward limit for dive to include rotation (eg bird diving towards player when player is above bird)
        static constexpr float UPWARD_LIMIT = 50.f; 

        if (mgr.HasComponent(entity, TransformType)) {
            float dy = playerPos.y - myPos.y;
            if (dy > UPWARD_LIMIT) {
               
                // atan2 zero = +X.  Sprite zero = -Y.  Offset = +PI/2.
                float angleRad = std::atan2(direction_.y, direction_.x) + static_cast<float>(M_PI * 0.5f);
                mgr.GetComponent<Transform>(entity).rotation.textureAngle = angleRad;
            }
            else {
                // Player is at the same level or below  the dive sprite
                // already points downward, which is correct.  No rotation.
                mgr.GetComponent<Transform>(entity).rotation.textureAngle = 0.0f;
            }
        }

        MessageBus::Instance().Publish(BirdAttackEvent(entity, myPos));
    }
    void BirdAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);
        if (!mgr.HasComponent(entity, VelocityType)) return;

        Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;

        // ---------------------------------------------------------------
        // Accelerate along the dive direction, capped at attackMaxSpeed_
        // This gives the "lunge" feel instead of instant full speed.
        // ---------------------------------------------------------------
        vel += direction_ * attackAccel_ * dt;

        // Clamp total speed to max
        float speed = vel.Magnitude();
        if (speed > attackMaxSpeed_) {
            vel = vel * (attackMaxSpeed_ / speed);
        }

      
            
        
    }
    void BirdAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackSpeed")    attackMaxSpeed_ = value;   // JSON key kept same
        if (key == "attackAccel")    attackAccel_ = value;
        if (key == "duration")       duration_ = value;
    }

    // --- RECOVERY ---
    void BirdRecoveryState::OnEnter(Entity entity, EntityStorage& ecs) {
        ownerID_ = entity;
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }

        if (mgr.HasComponent(entity, TransformType)) {
            Transform transform = mgr.GetComponent<Transform>(entity);
            transform.rotation.textureAngle = 0.0f;
        }


        
    }
    void BirdRecoveryState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)ecs;
        timer_ += dt;
    }
    void BirdRecoveryState::SetParameter(const std::string& key, float value) {
        if (key == "duration") duration_ = value;
    }

    // --- DEATH ---
    void BirdDeathState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity;
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        Vector2D deathPosition = { 0.0f, 0.0f };
        if (mgr.HasComponent(entity, TransformType)) {
            deathPosition = mgr.GetComponent<Transform>(entity).position;
        }

        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
        }

        if (mgr.HasComponent(entity, RigidBodyType)) {
            mgr.GetComponent<RigidBody>(entity).SetMass(0.0f);
        }
        if (mgr.HasComponent(entity, TextureComponentType)) {
            mgr.GetComponent<TextureComponent>(entity).textureKey.clear();
        }

        if (mgr.HasComponent(entity, AnimationComponentType)) {
            mgr.GetComponent<AnimationComponent>(entity).isPlaying = false;
        }
        if (mgr.HasComponent(entity, AnimationComponentType)) {
            mgr.GetComponent<AnimationComponent>(entity).currentState = "DEATH";
        }

        s_lastChaseVel.erase(entity);  // cleanup

        MessageBus::Instance().Publish(VFXTriggerEvent{ "EnemyDeathEvent",true, deathPosition });

        Logger::Get().Info(DebugType::AISystem) << "Enemy death VFX triggered at (" << deathPosition.x << ", " << deathPosition.y << ")" << std::endl;
    }

    void BirdDeathState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)ecs;
        timer_ += dt;
    }

    void BirdDeathState::SetParameter(const std::string& key, float value) {
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

    // --- WITCH TELEPORT SPIN ---
    void WitchTeleportSpinState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        // Stop velocity during spin
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchTeleportSpinState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
    }

    void WitchTeleportSpinState::SetParameter(const std::string& key, float value) {
        if (key == "spinDuration") spinDuration_ = value;
    }

    // --- WITCH TELEPORT FALL ---
    void WitchTeleportFallState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        Entity player = GetPlayer(ecs);
        if (player == 0 || !mgr.HasComponent(entity, TransformType) ||
            !mgr.HasComponent(player, TransformType)) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;

        // Pick a random angle and distance within range of the player
        static std::mt19937 rng{ std::random_device{}() };
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
        std::uniform_real_distribution<float> distDist(minTeleportRange_, maxTeleportRange_);

        float angle = angleDist(rng);
        float dist = distDist(rng);

        Vector2D offset = { std::cos(angle) * dist, std::sin(angle) * dist };
        Vector2D teleportPos = playerPos + offset;

        // Instantly teleport to new position
        mgr.GetComponent<Transform>(entity).position = teleportPos;

        // Stop velocity
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchTeleportFallState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
    }

    void WitchTeleportFallState::SetParameter(const std::string& key, float value) {
        if (key == "fallDuration") fallDuration_ = value;
        if (key == "minTeleportRange") minTeleportRange_ = value;
        if (key == "maxTeleportRange") maxTeleportRange_ = value;
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
        currentRadius_ = 0.0f;
        damageDealt_ = false;

        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);

        // Expanding/contracting circle: expand first half, contract second half
        float halfDuration = attackDuration_ * 0.5f;
        if (timer_ <= halfDuration) {
            // Expanding phase: 0 -> maxAttackRadius_
            currentRadius_ = (timer_ / halfDuration) * maxAttackRadius_;
        }
        else {
            // Contracting phase: maxAttackRadius_ -> 0
            float contractT = (timer_ - halfDuration) / halfDuration;
            if (contractT > 1.0f) contractT = 1.0f;
            currentRadius_ = (1.0f - contractT) * maxAttackRadius_;
        }

        // Check if player is inside the expanding circle to deal damage
        if (!damageDealt_) {
            Entity player = GetPlayer(ecs);
            if (player != 0 && mgr.HasComponent(entity, TransformType) &&
                mgr.HasComponent(player, TransformType)) {

                Vector2D myPos = mgr.GetComponent<Transform>(entity).position;
                Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
                float distance = myPos.Distance(playerPos);

                if (distance <= currentRadius_) {
                    damageDealt_ = true;

                    // Hurt the player if they have PlayerState
                    if (mgr.HasComponent(player, PlayerStateType)) {
                        PlayerState& ps = mgr.GetComponent<PlayerState>(player);
                        if (!ps.isHurt && !ps.isDead) {
                            ps.isHurt = true;
                            ps.invulnerabilityTimer = 1.5f;
                            ps.currentLives--;
                        }
                    }
                }
            }
        }

        // Write current radius to FSMComponent for visual rendering
        if (mgr.HasComponent(entity, FSMComponentType)) {
            mgr.GetComponent<FSMComponent>(entity).attackCircleRadius = currentRadius_;
        }
    }

    void WitchAttackState::OnExit(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, FSMComponentType)) {
            mgr.GetComponent<FSMComponent>(entity).attackCircleRadius = 0.0f;
        }
        currentRadius_ = 0.0f;
    }

    void WitchAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackDuration") attackDuration_ = value;
        if (key == "attackRadius") maxAttackRadius_ = value;
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
    REGISTER_STATE("Death", BirdDeathState)

    // witch registration 
    REGISTER_STATE("WitchIdle", WitchIdleState);
    REGISTER_STATE("WitchAlerted", WitchAlertedState);
    REGISTER_STATE("WitchEnraged", WitchEnragedState);
    REGISTER_STATE("WitchTeleportSpin", WitchTeleportSpinState);
    REGISTER_STATE("WitchTeleportFall", WitchTeleportFallState);
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
        (void)data;
        std::string sName = data["state"].GetString();
        return [sName](Entity e, EntityStorage& ecs) {
            EntityManager mgr(ecs);
            FSMComponent& fsm = mgr.GetComponent<FSMComponent>(e);
            State* s = fsm.fsm.GetState(sName);

            if (auto* ws = dynamic_cast<BirdWakingState*>(s))
                return ws->GetTimer() >= ws->GetWakeUpTime();
            if (auto* bs = dynamic_cast<BirdBounceState*>(s))
                return bs->GetTimer() >= bs->GetDuration();
            if (auto* as = dynamic_cast<BirdAttackState*>(s))
                return as->GetTimer() >= as->GetDuration();
            if (auto* rs = dynamic_cast<BirdRecoveryState*>(s))
                return rs->GetTimer() >= rs->GetDuration();

            // --- WITCH STATES ---
            if (auto* wa = dynamic_cast<WitchAlertedState*>(s))
                return wa->GetTimer() >= wa->GetAlertDuration();
            if (auto* we = dynamic_cast<WitchEnragedState*>(s))
                return we->GetTimer() >= we->GetEnrageDuration();
            if (auto* wt = dynamic_cast<WitchTeleportSpinState*>(s))
                return wt->GetTimer() >= wt->GetSpinDuration();
            if (auto* wtf = dynamic_cast<WitchTeleportFallState*>(s))
                return wtf->GetTimer() >= wtf->GetFallDuration();
            if (auto* wc = dynamic_cast<WitchChargingState*>(s))
                return wc->GetTimer() >= wc->GetChargeDuration();
            if (auto* watt = dynamic_cast<WitchAttackState*>(s))
                return watt->GetTimer() >= watt->GetAttackDuration();

            return false;
        };
    });


        // AIStates.cpp, improved collision detection
    REGISTER_CONDITION(HitWall, [](const rapidjson::Value& data) {
        (void)data;
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
            auto platforms = mgr.GetAllEntitiesWithComponent(true, PlatformTagType);

            // Check if  overlapping with any platform
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
