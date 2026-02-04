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
#include "OwalaTime.h"

namespace Framework {

    // --- SHARED AUDIO HELPERS ---
    // Tracks the absolute time (in seconds) the last vocal event occurred.
    static float s_LastVocalTime = -100.0f;

    // Minimum seconds between ANY chase or attack screams
    const float kVocalCooldown = 2.0f;

    // Helper to check if we can play audio
    static bool CanPlayVocalEvent() {
        float currentTime = Time::TotalRuntime();

        if (currentTime - s_LastVocalTime >= kVocalCooldown) {
            s_LastVocalTime = currentTime;
            return true;
        }
        return false;
    }

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

        if (CanPlayVocalEvent()) {
            MessageBus::Instance().Publish(BirdWakeEvent());
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
        (void)ecs;
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType)) {
            s_lastChaseVel[entity] = mgr.GetComponent<Velocity>(entity).Vel;
        }

        if (!hasStartedChase_ && CanPlayVocalEvent()) {
            MessageBus::Instance().Publish(BirdChaseEvent(true));
            hasStartedChase_ = true; // Lock it so it doesn't fire on Bounce -> Chase
        }
    }

    void BirdSimpleChaseState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)dt;

        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);

        if (player == 0 || !mgr.HasComponent(entity, TransformType) ||
            !mgr.HasComponent(player, TransformType)) return;

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
        // VERTICAL  �  accelerate toward targetY
        //   Use the slower accel when climbing up, faster when swooping
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

        // ---------------------------------------------------------------
        // Cache velocity BEFORE physics resolves collisions next frame
        // (Bounce needs the pre-zero X value)
        // ---------------------------------------------------------------
        s_lastChaseVel[entity] = vel;

        // ---------------------------------------------------------------
        // SPRITE ROTATION  �  face the player (not movement dir)
        // This gives the classic "eye tracking" look.
        // textureAngle is in radians.
        // ---------------------------------------------------------------
        //if (mgr.HasComponent(entity, TransformType)) {
        //    float dx = targetPos.x - myPos.x;
        //    float dy = targetPos.y - myPos.y;
        //    if (std::abs(dx) > 0.1f || std::abs(dy) > 0.1f) {

        //        float angleRad = std::atan2(dy, dx);

        //        // 2. Add offset because sprite faces Left natively (+180 degrees)
        //        float finalAngle = angleRad + 3.14159265f;

        //        // 3. FIX: If moving Left, AnimationSystem flips the sprite. 
        //        // We must subtract 180 (PI) to compensate, or it will face the wrong way.
        //        if (vel.x < 0.0f) {
        //            finalAngle -= 3.14159265f;
        //        }

        //        // 4. Convert to Degrees (Engine uses degrees for Transform)
        //       
        //        float angleDeg = finalAngle * (180.0f / 3.14159265f);

        //        mgr.GetComponent<Transform>(entity).rotation.textureAngle = angleRad;
        //    }
        //}

    }

    void BirdSimpleChaseState::OnExit(Entity entity, EntityStorage& ecs) {
        (void)entity;
        (void)ecs;

        // pause the looping audio
        MessageBus::Instance().Publish(BirdChaseEvent(false));
    }
    
    void BirdSimpleChaseState::SetParameter(const std::string& key, float value) {
        if (key == "accelX")         accelX_ = value;
        if (key == "accelYUp")       accelYUp_ = value;
        if (key == "accelYDown")     accelYDown_ = value;
        if (key == "maxSpeedX")      maxSpeedX_ = value;
        if (key == "maxSpeedY")      maxSpeedY_ = value;
        if (key == "hoverOffset")    hoverOffset_ = value;
        if (key == "turnaroundMult") turnaroundMult_ = value;
        // Legacy keys (ignored but won't crash if still in JSON)
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
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        if (!mgr.HasComponent(entity, VelocityType)) return;

        // Get current velocity to determine bounce direction
        Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;

        // ----------------------------------------------------------
       // Read the PRE-collision velocity from the cache.
       // PhysicsSystem already zeroed vel.x by the time we get here,
       // so we cannot trust the current vel.x for the bounce calc.
       // ----------------------------------------------------------
        Vector2D preVel = { 0.0f, 0.0f };
        auto it = s_lastChaseVel.find(entity);
        if (it != s_lastChaseVel.end()) {
            preVel = it->second;
        }

        // Determine which side we hit based on the cached X velocity
        // positive X  was moving right  hit a right wall  normal is -1
        collisionNormalX_ = (preVel.x >= 0.0f) ? -1.0f : 1.0f;

        // ----------------------------------------------------------
        // REFLECT X, retain bounceFactor_ of the speed
        // Keep Y as-is (Demon Eye only bounces horizontally off walls)
        // ----------------------------------------------------------
        vel.x = -preVel.x * bounceFactor_;
        // Y: keep whatever physics left us with, but use cached if
        // physics zeroed it too (floor/ceiling hit)
        if (std::abs(vel.y) < 0.1f) {
            vel.y = preVel.y;  // preserve Y momentum
        }

        // ----------------------------------------------------------
        // NUDGE out of the wall so physics doesn't immediately
        // re-resolve the same collision next frame
        // ----------------------------------------------------------
        if (mgr.HasComponent(entity, TransformType)) {
            mgr.GetComponent<Transform>(entity).position.x +=
                collisionNormalX_ * nudgeAmount_;
            // Reset texture rotation � Bounce uses the fly animation flat
            mgr.GetComponent<Transform>(entity).rotation.textureAngle = 0.0f;
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
                // Player is ABOVE the bird � the default dive sprite points down,
                // so we need to rotate to point toward the player.
                // atan2 zero = +X.  Sprite zero = -Y.  Offset = +PI/2.
                float angleRad = std::atan2(direction_.y, direction_.x) + static_cast<float>(M_PI * 0.5f);
                mgr.GetComponent<Transform>(entity).rotation.textureAngle = angleRad;
            }
            else {
                // Player is at the same level or below � the dive sprite
                // already points downward, which is correct.  No rotation.
                mgr.GetComponent<Transform>(entity).rotation.textureAngle = 0.0f;
            }
        }

        if (CanPlayVocalEvent()) {
            MessageBus::Instance().Publish(BirdAttackEvent());
        }
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

        // ---------------------------------------------------------------
        // SPRITE ROTATION  �  tilt to match ACTUAL velocity direction.
        // During a dive this naturally points nose-down.
        // textureAngle is radians.
        // ---------------------------------------------------------------
            //if (mgr.HasComponent(entity, TransformType)) {
            //    if (vel.x != 0.0f || vel.y != 0.0f) {
            //        // direction_ is the snapshotted normalised vector toward player
            //  // (set in OnEnter, does not change during the dive)
            //        float angleRad = std::atan2(direction_.y, direction_.x);

            //        // +PI/2 offset because the dive sprite's default pose points DOWN (-Y),
            //        // but atan2's zero is +X (RIGHT).  The difference is 90� = PI/2.
            //        angleRad += 3.14159265f * 0.5f;   // + PI/2

            //        mgr.GetComponent<Transform>(entity).rotation.textureAngle = angleRad;
            //    }
            //}

            
        
    }
    void BirdAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackSpeed")    attackMaxSpeed_ = value;   // JSON key kept same
        if (key == "attackAccel")    attackAccel_ = value;
        if (key == "duration")       duration_ = value;
    }

    // --- RECOVERY ---
    void BirdRecoveryState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, VelocityType))
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        if (mgr.HasComponent(entity, TransformType))
            mgr.GetComponent<Transform>(entity).rotation.textureAngle = 0.0f;
        }

        // Reset the chase flag
        if (mgr.HasComponent(entity, FSMComponentType)) {
            FSM& fsm = mgr.GetComponent<FSMComponent>(entity).fsm;

            // Retrieve the SimpleChase state and reset it
            auto* chaseState = dynamic_cast<BirdSimpleChaseState*>(fsm.GetState("SimpleChase"));
            if (chaseState) {
                chaseState->ResetChaseFlag();
            }
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
            mgr.GetComponent<AnimationComponent>(entity).currentState = AnimationState::DEATH;
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
    // MACRO REGISTRATION FOR STATES
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

            return false;
        };
    });

       
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
