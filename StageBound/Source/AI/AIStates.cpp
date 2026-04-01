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
#include "GameMessages.h"
#include "CameraSystem.h"
#include "TransformationSystem.h"
#include <algorithm>

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

        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        
        // Add RotationFlag so bird's position rotates with the world like platforms during idle
        if (!mgr.HasComponent(entity, RotationFlagType)) {
            RotationFlag rotFlag;
            if (transformSys) {
                rotFlag.currentRotation = transformSys->GetGlobalRotation();
            }
            mgr.AddComponent<RotationFlag>(entity, rotFlag);
        } else {
            // Update existing RotationFlag to match current world rotation
            if (transformSys) {
                mgr.GetComponent<RotationFlag>(entity).currentRotation = transformSys->GetGlobalRotation();
            }
        }

        s_lastChaseVel.erase(entity);

    }
    void BirdIdleState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)dt;
        EntityManager mgr(ecs);
        
        // Continuously sync RotationFlag with world rotation during idle
        // (since enemies are skipped by TransformationSystem's automatic rotation update)
        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        if (transformSys && mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = transformSys->GetGlobalRotation();
        }
    }
    void BirdIdleState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpRadius") wakeUpRadius_ = value;
        
    }

    // --- WAKING ---
    void BirdWakingState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;

        EntityManager mgr(ecs);
        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        
        // Ensure bird still has RotationFlag during waking (rotates with world)
        if (!mgr.HasComponent(entity, RotationFlagType)) {
            RotationFlag rotFlag;
            if (transformSys) {
                rotFlag.currentRotation = transformSys->GetGlobalRotation();
            }
            mgr.AddComponent<RotationFlag>(entity, rotFlag);
        } else {
            // Update existing RotationFlag to match current world rotation
            if (transformSys) {
                mgr.GetComponent<RotationFlag>(entity).currentRotation = transformSys->GetGlobalRotation();
            }
        }
        
        if (mgr.HasComponent(entity, TransformType)) {
            Vector2D position = mgr.GetComponent<Transform>(entity).position;
            MessageBus::Instance().Publish(BirdWakeEvent(entity, position));
            MessageBus::Instance().Publish(PlaySound3DEvent{ "Bird_Wake", AudioType::SFX, 0.3f, position, 50.0f, 700.0f, entity });
        }
    }
    void BirdWakingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);
        
        // Continuously sync RotationFlag with world rotation during waking
        // (since enemies are skipped by TransformationSystem's automatic rotation update)
        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        if (transformSys && mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = transformSys->GetGlobalRotation();
        }
    }
    void BirdWakingState::SetParameter(const std::string& key, float value) {
        if (key == "wakeUpTime") wakeUpTime_ = value;
    }

    // --- SIMPLE CHASE ---
    void BirdSimpleChaseState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        ownerID_ = entity;

        // Remove RotationFlag so bird no longer rotates with world - it's now flying freely
        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
           // mgr.RemoveComponent(entity, RotationFlagType);
        }

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

        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
            // mgr.RemoveComponent(entity, RotationFlagType);
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
            Transform& transform = mgr.GetComponent<Transform>(entity);
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

        // Remove RotationFlag so bird is not affected by world rotation during bounce
        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
           // mgr.RemoveComponent(entity, RotationFlagType);
        }

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
            Transform& transform = mgr.GetComponent<Transform>(entity);
            transform.position.x += collisionNormalX_ * nudgeAmount_;
            // Reset texture rotation, Bounce uses the fly animation flat
            transform.rotation.textureAngle = 0.0f;
        }
    }

    void BirdBounceState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);

        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
            // mgr.RemoveComponent(entity, RotationFlagType);
        }

        // Gradually reduce bounce velocity
        
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
    void BirdHoverState::OnEnter(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        
      
    }

    void BirdHoverState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        attackTimer_ -= dt;
        EntityManager mgr(ecs);
        Entity player = GetPlayer(ecs);
        if (player == 0) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
        Vector2D myPos = mgr.GetComponent<Transform>(entity).position;

        // Remove RotationFlag so bird is not affected by world rotation during hover
        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
            // mgr.RemoveComponent(entity, RotationFlagType);
        }

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

        // Remove RotationFlag so bird is not affected by world rotation during attack
       

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
        MessageBus::Instance().Publish(PlaySound3DEvent{ "Bird_Attack", AudioType::SFX, 0.3f, myPos, 50.0f, 700.0f, entity });
    }
    void BirdAttackState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);
        if (!mgr.HasComponent(entity, VelocityType)) return;

        Vector2D& vel = mgr.GetComponent<Velocity>(entity).Vel;
        mgr.GetComponent<RotationFlag>(entity).currentRotation = 0.0f;
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

        // Remove RotationFlag so bird is not affected by world rotation during recovery
        if (mgr.HasComponent(entity, RotationFlagType)) {
            mgr.RemoveComponent(entity, RotationFlagType);
        }

        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }

        if (mgr.HasComponent(entity, TransformType)) {
            Transform& transform = mgr.GetComponent<Transform>(entity);
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
        (void)entity; (void)ecs; (void)dt; 
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
        if (mgr.HasComponent(entity, TransformType)) {
            Vector2D position = mgr.GetComponent<Transform>(entity).position;
            MessageBus::Instance().Publish(PlaySound3DEvent{ "Witch_Alert", AudioType::SFX, 0.3f, position });
        }
    }

    void WitchAlertedState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; (void)ecs;
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
        (void)entity; (void)ecs;
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
        if (mgr.HasComponent(entity, TransformType)) {
            Vector2D position = mgr.GetComponent<Transform>(entity).position;
            MessageBus::Instance().Publish(PlaySound3DEvent{ "Witch_Teleport", AudioType::SFX, 0.3f, position });
        }
    }

    void WitchTeleportSpinState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; (void)ecs;
        timer_ += dt;
    }

    void WitchTeleportSpinState::SetParameter(const std::string& key, float value) {
        if (key == "spinDuration") spinDuration_ = value;
    }

    // --- WITCH TELEPORT FALL ---
    // Helper: Get the effective "up" direction based on world rotation
    static Vector2D GetEffectiveUpDirection(float worldRotationDeg) {
        // Normalize to 0-360
        float rot = std::fmod(worldRotationDeg + 360.0f, 360.0f);
        
        // Round to nearest 90 degree increment
        if (rot >= 315.0f || rot < 45.0f) {
            return { 0.0f, 1.0f };  // Normal up
        } else if (rot >= 45.0f && rot < 135.0f) {
            return { -1.0f, 0.0f }; // Up is to the left (rotated 90° CCW)
        } else if (rot >= 135.0f && rot < 225.0f) {
            return { 0.0f, -1.0f }; // Up is down (rotated 180°)
        } else {
            return { 1.0f, 0.0f };  // Up is to the right (rotated 270°)
        }
    }

    // Helper: Calculate platform top position based on rotation
    static Vector2D GetPlatformTopPosition(const Vector2D& platformPos, const Vector2D& halfSize, 
                                           float platformRotation, float witchHalfHeight, 
                                           float worldRotation) {
        Vector2D upDir = GetEffectiveUpDirection(worldRotation);
        
        // Calculate the platform's rotated bounds
        AABB_Bounds bounds = PhysicsSystem::GetRotatedAABB(platformPos, halfSize, platformRotation);
        
        // The "top" depends on world rotation
        Vector2D result;
        if (std::abs(upDir.y) > 0.5f) {
            // Normal or inverted
            if (upDir.y > 0) {
                result = { platformPos.x, bounds.max.y + witchHalfHeight + 1.0f };
            } else {
                result = { platformPos.x, bounds.min.y - witchHalfHeight - 1.0f };
            }
        } else {
            // World rotated 90 or 270
            if (upDir.x > 0) {
                result = { bounds.max.x + witchHalfHeight + 1.0f, platformPos.y };
            } else {
                result = { bounds.min.x - witchHalfHeight - 1.0f, platformPos.y };
            }
        }
        return result;
    }

    void WitchTeleportFallState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        Entity player = GetPlayer(ecs);
        if (player == 0 || !mgr.HasComponent(entity, TransformType) ||
            !mgr.HasComponent(player, TransformType)) return;

        Vector2D playerPos = mgr.GetComponent<Transform>(player).position;
        
        // Use actual AABB collider size for witch, not transform scale
        Vector2D witchHS{ 50.0f, 50.0f }; // Default fallback
        if (mgr.HasComponent(entity, AABBColliderType)) {
            const AABBCollider& witchCollider = mgr.GetComponent<AABBCollider>(entity);
            witchHS = witchCollider.HalfSize;
        }

        // Get world rotation from TransformationSystem
        float worldRotation = 0.0f;
        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        if (transformSys) {
            worldRotation = transformSys->GetGlobalRotation();
        }

        // Get camera viewport bounds to ensure witch stays visible
        CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
        float viewLeft = -1000.0f, viewRight = 1000.0f, viewBottom = -1000.0f, viewTop = 1000.0f;
        if (camSys) {
            Entity activeCamera = camSys->GetActiveCamera();
            if (activeCamera != 0) {
                camSys->GetViewBounds(activeCamera, viewLeft, viewRight, viewBottom, viewTop);
            }
        }
        // Add padding so witch doesn't appear at the very edge
        const float viewPadding = 50.0f;
        viewLeft += viewPadding + witchHS.x;
        viewRight -= viewPadding + witchHS.x;
        viewBottom += viewPadding + witchHS.y;
        viewTop -= viewPadding + witchHS.y;

        // Gather all platform entities (entities with PlatformTag or Platform layer)
        std::vector<Entity> platforms;
        for (Entity e : mgr.GetAllEntitiesWithComponent(true, AABBColliderType)) {
            if (e == entity || e == player) continue;
            if (!mgr.HasComponent(e, TransformType)) continue;
            if (!mgr.HasComponent(e, LoadedTagType)) continue;
            
            // Check if this is a platform (has PlatformTag or Platform collision layer)
            bool isPlatform = mgr.HasComponent(e, PlatformTagType);
            if (!isPlatform) {
                const EntityProperties& props = mgr.GetProperties(e);
                isPlatform = (props.layer == CollisionLayer::Platform);
            }
            
            if (isPlatform) {
                platforms.push_back(e);
            }
        }

        // Find valid teleport positions on platforms near the player
        struct PlatformCandidate {
            Entity platformEntity;
            Vector2D topPosition;
            float distToPlayer;
        };
        std::vector<PlatformCandidate> candidates;

        for (Entity plat : platforms) {
            const Transform& platTf = mgr.GetComponent<Transform>(plat);
            const AABBCollider& platCollider = mgr.GetComponent<AABBCollider>(plat);
            
            float platformRotation = 0.0f;
            if (mgr.HasComponent(plat, RotationFlagType)) {
                platformRotation = mgr.GetComponent<RotationFlag>(plat).currentRotation;
            }

            Vector2D platformPos = platTf.position + platCollider.Offset;
            
            // Get the top position of this platform
            Vector2D topPos = GetPlatformTopPosition(platformPos, platCollider.HalfSize, 
                                                     platformRotation, witchHS.y, worldRotation);
            
            // Check if within range
            float dist = topPos.Distance(playerPos);
            if (dist < minTeleportRange_ || dist > maxTeleportRange_) continue;
            
            // Check if within viewport
            if (topPos.x < viewLeft || topPos.x > viewRight ||
                topPos.y < viewBottom || topPos.y > viewTop) continue;
            
            candidates.push_back({ plat, topPos, dist });
        }

        // Sort by distance to player
        std::sort(candidates.begin(), candidates.end(), 
            [](const PlatformCandidate& a, const PlatformCandidate& b) {
                return a.distToPlayer < b.distToPlayer;
            });

        // Gather all collidables for overlap testing
        auto collidables = mgr.GetAllEntitiesWithComponent(true, AABBColliderType);
        
        Vector2D teleportPos = mgr.GetComponent<Transform>(entity).position;
        bool foundValidPos = false;

        // Try each candidate platform position
        for (const auto& candidate : candidates) {
            // Try multiple X positions along the platform top
            const Transform& platTf = mgr.GetComponent<Transform>(candidate.platformEntity);
            const AABBCollider& platCollider = mgr.GetComponent<AABBCollider>(candidate.platformEntity);
            
            float platformRotation = 0.0f;
            if (mgr.HasComponent(candidate.platformEntity, RotationFlagType)) {
                platformRotation = mgr.GetComponent<RotationFlag>(candidate.platformEntity).currentRotation;
            }
            
            AABB_Bounds platBounds = PhysicsSystem::GetRotatedAABB(
                platTf.position + platCollider.Offset, platCollider.HalfSize, platformRotation);
            
            Vector2D upDir = GetEffectiveUpDirection(worldRotation);
            
            // Generate positions along the platform surface
            std::vector<Vector2D> positionsToTry;
            
            if (std::abs(upDir.y) > 0.5f) {
                // Normal or inverted - move along X axis
                float yPos = (upDir.y > 0) ? (platBounds.max.y + witchHS.y + 1.0f) 
                                           : (platBounds.min.y - witchHS.y - 1.0f);
                float xMin = platBounds.min.x + witchHS.x;
                float xMax = platBounds.max.x - witchHS.x;
                
                // Prefer position closest to player
                float playerX = std::clamp(playerPos.x, xMin, xMax);
                positionsToTry.push_back({ playerX, yPos });
                positionsToTry.push_back({ (xMin + xMax) * 0.5f, yPos }); // Center
                positionsToTry.push_back({ xMin, yPos });
                positionsToTry.push_back({ xMax, yPos });
            } else {
                // World rotated 90 or 270 - move along Y axis
                float xPos = (upDir.x > 0) ? (platBounds.max.x + witchHS.y + 1.0f) 
                                           : (platBounds.min.x - witchHS.y - 1.0f);
                float yMin = platBounds.min.y + witchHS.x;
                float yMax = platBounds.max.y - witchHS.x;
                
                float playerY = std::clamp(playerPos.y, yMin, yMax);
                positionsToTry.push_back({ xPos, playerY });
                positionsToTry.push_back({ xPos, (yMin + yMax) * 0.5f });
                positionsToTry.push_back({ xPos, yMin });
                positionsToTry.push_back({ xPos, yMax });
            }
            
            for (const Vector2D& candidatePos : positionsToTry) {
                // Check viewport bounds
                if (candidatePos.x < viewLeft || candidatePos.x > viewRight ||
                    candidatePos.y < viewBottom || candidatePos.y > viewTop) continue;
                
                // Check distance from player
                float dist = candidatePos.Distance(playerPos);
                if (dist < minTeleportRange_ || dist > maxTeleportRange_) continue;
                
                // Build AABB for witch at candidate position
                AABB_Bounds witchBounds = {
                    { candidatePos.x - witchHS.x, candidatePos.y - witchHS.y },
                    { candidatePos.x + witchHS.x, candidatePos.y + witchHS.y }
                };

                bool overlaps = false;
                for (Entity collidable : collidables) {
                    if (collidable == entity) continue;
                    if (collidable == candidate.platformEntity) continue; // Allow touching the platform we're standing on
                    if (!mgr.HasComponent(collidable, TransformType)) continue;
                    if (!mgr.HasComponent(collidable, AABBColliderType)) continue;

                    const Transform& ct = mgr.GetComponent<Transform>(collidable);
                    const AABBCollider& collider = mgr.GetComponent<AABBCollider>(collidable);
                    Vector2D colliderPos = ct.position + collider.Offset;

                    float colliderRotation = 0.0f;
                    if (mgr.HasComponent(collidable, RotationFlagType)) {
                        colliderRotation = mgr.GetComponent<RotationFlag>(collidable).currentRotation;
                    }
                    
                    AABB_Bounds collidableBounds = PhysicsSystem::GetRotatedAABB(colliderPos, collider.HalfSize, colliderRotation);

                    if (CollisionIntersection_RectRect_Static(witchBounds, collidableBounds)) {
                        overlaps = true;
                        break;
                    }
                }

                if (!overlaps) {
                    teleportPos = candidatePos;
                    foundValidPos = true;
                    break;
                }
            }
            
            if (foundValidPos) break;
        }

        // Fallback: if no platform position found, try original random approach but avoid overlap
        if (!foundValidPos) {
            static std::mt19937 rng{ std::random_device{}() };
            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
            std::uniform_real_distribution<float> distDist(minTeleportRange_, maxTeleportRange_);

            const int maxAttempts = 30;
            for (int attempt = 0; attempt < maxAttempts; ++attempt) {
                float angle = angleDist(rng);
                float dist = distDist(rng);
                Vector2D candidatePos = playerPos + Vector2D{ std::cos(angle) * dist, std::sin(angle) * dist };

                if (candidatePos.x < viewLeft || candidatePos.x > viewRight ||
                    candidatePos.y < viewBottom || candidatePos.y > viewTop) continue;

                AABB_Bounds witchBounds = {
                    { candidatePos.x - witchHS.x, candidatePos.y - witchHS.y },
                    { candidatePos.x + witchHS.x, candidatePos.y + witchHS.y }
                };

                bool overlaps = false;
                for (Entity collidable : collidables) {
                    if (collidable == entity) continue;
                    if (!mgr.HasComponent(collidable, TransformType)) continue;
                    if (!mgr.HasComponent(collidable, AABBColliderType)) continue;

                    const Transform& ct = mgr.GetComponent<Transform>(collidable);
                    const AABBCollider& collider = mgr.GetComponent<AABBCollider>(collidable);
                    Vector2D colliderPos = ct.position + collider.Offset;

                    float colliderRotation = 0.0f;
                    if (mgr.HasComponent(collidable, RotationFlagType)) {
                        colliderRotation = mgr.GetComponent<RotationFlag>(collidable).currentRotation;
                    }
                    
                    AABB_Bounds collidableBounds = PhysicsSystem::GetRotatedAABB(colliderPos, collider.HalfSize, colliderRotation);

                    if (CollisionIntersection_RectRect_Static(witchBounds, collidableBounds)) {
                        overlaps = true;
                        break;
                    }
                }

                if (!overlaps) {
                    teleportPos = candidatePos;
                    foundValidPos = true;
                    break;
                }
            }
        }

        // If no valid position found, clamp current position to viewport at minimum
        if (!foundValidPos) {
            teleportPos.x = std::clamp(teleportPos.x, viewLeft, viewRight);
            teleportPos.y = std::clamp(teleportPos.y, viewBottom, viewTop);
        }

        // Teleport to valid position
        mgr.GetComponent<Transform>(entity).position = teleportPos;

       

        // Play teleport sound at new position
        MessageBus::Instance().Publish(PlaySound3DEvent{ "Witch_Teleport", AudioType::SFX, 0.3f, teleportPos });

        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchTeleportFallState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; //(void)ecs;
        timer_ += dt;
        EntityManager mgr(ecs);
        RigidBody rb;
        rb.isAffectedByGravity = false;

        mgr.GetComponent<RigidBody>(entity) = rb; // Reset velocity and gravity
    }

    void WitchTeleportFallState::SetParameter(const std::string& key, float value) {
        if (key == "fallDuration") fallDuration_ = value;
        if (key == "minTeleportRange") minTeleportRange_ = value;
        if (key == "maxTeleportRange") maxTeleportRange_ = value;
    }

    // --- WITCH FALLING (gravity-based landing after teleport) ---
    void WitchFallingState::OnEnter(Entity entity, EntityStorage& ecs) {
        timer_ = 0.0f;
        isGrounded_ = false;
        EntityManager mgr(ecs);
        
        // Enable gravity for falling
        if (mgr.HasComponent(entity, RigidBodyType)) {
            RigidBody& rb = mgr.GetComponent<RigidBody>(entity);
            rb.isAffectedByGravity = true;
        }
        
        // Reset velocity
        if (mgr.HasComponent(entity, VelocityType)) {
            Velocity& vel = mgr.GetComponent<Velocity>(entity);
            vel.Vel = { 0.0f, 0.0f };
        }
    }

    void WitchFallingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        timer_ += dt;
        EntityManager mgr(ecs);
        
        // Get world rotation to determine "down" direction
        float worldRotation = 0.0f;
        TransformationSystem* transformSys = GameManager::GetInstance().GetSystems().transformation_;
        if (transformSys) {
            worldRotation = transformSys->GetGlobalRotation();
        }
        
        Vector2D downDir = { 0.0f, -1.0f };  // Default down
        float rot = std::fmod(worldRotation + 360.0f, 360.0f);
        if (rot >= 315.0f || rot < 45.0f) {
            downDir = { 0.0f, -1.0f };  // Normal down
        } else if (rot >= 45.0f && rot < 135.0f) {
            downDir = { 1.0f, 0.0f };   // Right (gravity pulls right)
        } else if (rot >= 135.0f && rot < 225.0f) {
            downDir = { 0.0f, 1.0f };   // Up (gravity pulls up)
        } else {
            downDir = { -1.0f, 0.0f };  // Left (gravity pulls left)
        }
        
        // Apply gravity in the "down" direction based on world rotation
        if (mgr.HasComponent(entity, VelocityType)) {
            Velocity& vel = mgr.GetComponent<Velocity>(entity);
            vel.Vel += downDir * fallSpeed_ * dt;
            
            // Clamp fall speed
            float speed = vel.Vel.Magnitude();
            if (speed > fallSpeed_) {
                vel.Vel = vel.Vel * (fallSpeed_ / speed);
            }
        }
        
        // Get witch position and use TextureComponent half_size for collision detection
        // (witch doesn't have an AABBCollider)
        if (mgr.HasComponent(entity, TransformType) && mgr.HasComponent(entity, TextureComponentType)) {
            Vector2D witchPos = mgr.GetComponent<Transform>(entity).position;
            const TextureComponent& witchTex = mgr.GetComponent<TextureComponent>(entity);
            Vector2D witchHS = witchTex.HalfSize;
            
            // Create a small probe box slightly below the witch to check for ground
            Vector2D probeOffset = downDir * 5.0f;  // Small distance in down direction
            AABB_Bounds witchBounds = {
                { witchPos.x - witchHS.x * 0.5f, witchPos.y - witchHS.y },
                { witchPos.x + witchHS.x * 0.5f, witchPos.y + witchHS.y * 0.2f }
            };
            
            // Offset by probe in down direction for ground check
            AABB_Bounds probeBounds = {
                { witchBounds.min.x + probeOffset.x, witchBounds.min.y + probeOffset.y },
                { witchBounds.max.x + probeOffset.x, witchBounds.max.y + probeOffset.y }
            };
            
            // Check collision with platforms
            for (Entity e : mgr.GetAllEntitiesWithComponent(true, AABBColliderType)) {
                if (e == entity) continue;
                if (!mgr.HasComponent(e, TransformType)) continue;
                if (!mgr.HasComponent(e, LoadedTagType)) continue;
                
                // Check if this is a platform (not trap platform)
                bool isPlatform = mgr.HasComponent(e, PlatformTagType);
                if (!isPlatform) {
                    const EntityProperties& props = mgr.GetProperties(e);
                    isPlatform = (props.layer == CollisionLayer::Platform);
                }
                
                // Exclude trap platforms
                const EntityProperties& eProps = mgr.GetProperties(e);
                if (eProps.layer == CollisionLayer::TrapPlatform) continue;
                
                if (!isPlatform) continue;
                
                const Transform& platTf = mgr.GetComponent<Transform>(e);
                const AABBCollider& platCollider = mgr.GetComponent<AABBCollider>(e);
                Vector2D platPos = platTf.position + platCollider.Offset;
                
                float platformRotation = 0.0f;
                if (mgr.HasComponent(e, RotationFlagType)) {
                    platformRotation = mgr.GetComponent<RotationFlag>(e).currentRotation;
                }
                
                AABB_Bounds platBounds = PhysicsSystem::GetRotatedAABB(platPos, platCollider.HalfSize, platformRotation);
                
                if (CollisionIntersection_RectRect_Static(probeBounds, platBounds)) {
                    isGrounded_ = true;
                    
                    // Stop falling
                    if (mgr.HasComponent(entity, VelocityType)) {
                        mgr.GetComponent<Velocity>(entity).Vel = { 0.0f, 0.0f };
                    }
                    
                    // Disable gravity
                    if (mgr.HasComponent(entity, RigidBodyType)) {
                        RigidBody& rb = mgr.GetComponent<RigidBody>(entity);
                        rb.isAffectedByGravity = false;
                    }
                    break;
                }
            }
        }
        
        // Timeout: if we've been falling too long, just proceed anyway
        if (timer_ >= maxFallTime_) {
            isGrounded_ = true;
            if (mgr.HasComponent(entity, VelocityType)) {
                mgr.GetComponent<Velocity>(entity).Vel = { 0.0f, 0.0f };
            }
            if (mgr.HasComponent(entity, RigidBodyType)) {
                RigidBody& rb = mgr.GetComponent<RigidBody>(entity);
                rb.isAffectedByGravity = false;
            }
        }
    }

    void WitchFallingState::SetParameter(const std::string& key, float value) {
        if (key == "maxFallTime") maxFallTime_ = value;
        if (key == "fallSpeed") fallSpeed_ = value;
    }

    // --- WITCH CHARGING ---
    void WitchChargingState::OnEnter(Entity entity, EntityStorage& ecs) {
        (void)entity;
        timer_ = 0.0f;
        EntityManager mgr(ecs);

        // Stop movement during charge
        if (mgr.HasComponent(entity, VelocityType)) {
            mgr.GetComponent<Velocity>(entity).Vel = { 0.f, 0.f };
        }
    }

    void WitchChargingState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity; (void)ecs;
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

        Vector2D position = { 0.0f, 0.0f };
        if (mgr.HasComponent(entity, TransformType)) {
            position = mgr.GetComponent<Transform>(entity).position;
        }
        MessageBus::Instance().Publish(PlaySound3DEvent{ "Witch_Attack", AudioType::SFX, 0.3f, position });

        if (mgr.HasComponent(entity, FSMComponentType)) {
            FSMComponent& fsm = mgr.GetComponent<FSMComponent>(entity);
            // Setup animated attack circle VFX
            fsm.attackCircleAnimated = true;
            fsm.attackCircleAnimTimer = 0.0f;
            fsm.attackCircleAnimFrame = 0;
            fsm.attackCircleAnimFrameDuration = 0.04f;
            fsm.attackCircleAnimTextures = {
                "SB_Witch_Circle_VFX_Spritesheet1.png",
                "SB_Witch_Circle_VFX_Spritesheet2.png",
                "SB_Witch_Circle_VFX_Spritesheet3.png",
                "SB_Witch_Circle_VFX_Spritesheet4.png",
                "SB_Witch_Circle_VFX_Spritesheet5.png",
                "SB_Witch_Circle_VFX_Spritesheet6.png",
                "SB_Witch_Circle_VFX_Spritesheet7.png",
                "SB_Witch_Circle_VFX_Spritesheet8.png",
                "SB_Witch_Circle_VFX_Spritesheet9.png",
                "SB_Witch_Circle_VFX_Spritesheet10.png",
                "SB_Witch_Circle_VFX_Spritesheet11.png",
                "SB_Witch_Circle_VFX_Spritesheet12.png",
                "SB_Witch_Circle_VFX_Spritesheet14.png",
                "SB_Witch_Circle_VFX_Spritesheet15.png",
                "SB_Witch_Circle_VFX_Spritesheet16.png",
                "SB_Witch_Circle_VFX_Spritesheet17.png",
                "SB_Witch_Circle_VFX_Spritesheet18.png",
                "SB_Witch_Circle_VFX_Spritesheet19.png",
                "SB_Witch_Circle_VFX_Spritesheet20.png",
                "SB_Witch_Circle_VFX_Spritesheet21.png",
                "SB_Witch_Circle_VFX_Spritesheet23.png",
                "SB_Witch_Circle_VFX_Spritesheet24.png",
                "SB_Witch_Circle_VFX_Spritesheet25.png",
                "SB_Witch_Circle_VFX_Spritesheet26.png",
                "SB_Witch_Circle_VFX_Spritesheet27.png"
            };
            // Start with first texture
            if (!fsm.attackCircleAnimTextures.empty()) {
                fsm.attackCircleTextureKey = fsm.attackCircleAnimTextures[0];
            }
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

                    // Route through PlayerSystem::DamagePlayer() via event so that
                    // knockback, invulnerability, VFX, and hurt/death sounds all fire
                    // consistently — the same path the bird uses.
                    MessageBus::Instance().Publish(PlayerDamageRequestEvent{
                        player,
                        myPos,
                        "witch attack"
                    });
                }
            }
        }

        // Write current radius to FSMComponent for visual rendering
        if (mgr.HasComponent(entity, FSMComponentType)) {
            FSMComponent& fsm = mgr.GetComponent<FSMComponent>(entity);
            fsm.attackCircleRadius = currentRadius_;
            
            // Update animated VFX frame
            if (fsm.attackCircleAnimated && !fsm.attackCircleAnimTextures.empty()) {
                fsm.attackCircleAnimTimer += dt;
                
                // Calculate total frames (4 frames per spritesheet)
                const int framesPerSheet = 4;
                int totalFrames = static_cast<int>(fsm.attackCircleAnimTextures.size()) * framesPerSheet;
                
                // Advance frame based on timer
                if (fsm.attackCircleAnimTimer >= fsm.attackCircleAnimFrameDuration) {
                    fsm.attackCircleAnimTimer -= fsm.attackCircleAnimFrameDuration;
                    fsm.attackCircleAnimFrame++;
                    
                    // Clamp to last frame (don't loop)
                    if (fsm.attackCircleAnimFrame >= totalFrames) {
                        fsm.attackCircleAnimFrame = totalFrames - 1;
                    }
                }
                
                // Determine which spritesheet and which sub-frame
                int sheetIndex = fsm.attackCircleAnimFrame / framesPerSheet;
                if (sheetIndex >= static_cast<int>(fsm.attackCircleAnimTextures.size())) {
                    sheetIndex = static_cast<int>(fsm.attackCircleAnimTextures.size()) - 1;
                }
                fsm.attackCircleTextureKey = fsm.attackCircleAnimTextures[sheetIndex];
            }
        }
    }

    void WitchAttackState::OnExit(Entity entity, EntityStorage& ecs) {
        EntityManager mgr(ecs);
        if (mgr.HasComponent(entity, FSMComponentType)) {
            FSMComponent& fsm = mgr.GetComponent<FSMComponent>(entity);
            fsm.attackCircleRadius = 0.0f;
            fsm.attackCircleTextureKey.clear();
            fsm.attackCircleAnimated = false;
            fsm.attackCircleAnimTimer = 0.0f;
            fsm.attackCircleAnimFrame = 0;
            fsm.attackCircleAnimTextures.clear();
        }
        currentRadius_ = 0.0f;
    }

    void WitchAttackState::SetParameter(const std::string& key, float value) {
        if (key == "attackDuration") attackDuration_ = value;
        if (key == "attackRadius") maxAttackRadius_ = value;
    }

    // --- WITCH DEATH ---
    void WitchDeathState::OnEnter(Entity entity, EntityStorage& ecs) {
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
            mgr.GetComponent<AnimationComponent>(entity).currentState = "DEATH";
        }

        MessageBus::Instance().Publish(VFXTriggerEvent{ "EnemyDeathEvent", true, deathPosition });
        MessageBus::Instance().Publish(PlaySound3DEvent{ "Witch_Death", AudioType::SFX, 0.3f, deathPosition });
        Logger::Get().Info(DebugType::AISystem) << "Witch death VFX triggered at (" << deathPosition.x << ", " << deathPosition.y << ")" << std::endl;
    }

    void WitchDeathState::OnUpdate(Entity entity, EntityStorage& ecs, float dt) {
        (void)entity;
        (void)ecs;
        timer_ += dt;
    }

    void WitchDeathState::SetParameter(const std::string& key, float value) {
        if (key == "duration") duration_ = value;
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
    REGISTER_STATE("WitchFalling", WitchFallingState);
    REGISTER_STATE("WitchCharging", WitchChargingState);
    REGISTER_STATE("WitchAttack", WitchAttackState);
    REGISTER_STATE("WitchDeath", WitchDeathState);

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
            if (auto* wfall = dynamic_cast<WitchFallingState*>(s))
                return wfall->IsGrounded() || wfall->GetTimer() >= wfall->GetMaxFallTime();
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
