/*********************************************************************//**
* \file   AIStates.h
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Declares the concrete state classes (BirdIdle, BirdWaking, BirdChase, etc.)
* derived from the base State class, defining their specific lifecycle
* methods and properties.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "FSM.h"
#include "Pathfinding.h"
#include "OwalaTime.h"
#include <vector>

namespace Framework {

    // Base class for states that need access to the Navigation Grid
    class NavState : public State {
    protected:
        NavigationGrid* navGrid_ = nullptr;
    public:
        void SetNavGrid(NavigationGrid* grid) override { navGrid_ = grid; }
    };

    // --- IDLE STATE ---
    class BirdIdleState : public State {
    public:
        const char* GetName() const override { return "Idle"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

    private:
        float wakeUpRadius_ = 300.0f;
    };

    // --- WAKING STATE ---
    class BirdWakingState : public State {
    public:
        const char* GetName() const override { return "Waking"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetWakeUpTime() const { return wakeUpTime_; }

    private:
        float timer_ = 0.0f;
        float wakeUpTime_ = 3.5f;
    };

    // --- SIMPLE CHASE STATE (No Pathfinding) ---
    class BirdSimpleChaseState : public State {
    public:
        const char* GetName() const override { return "SimpleChase"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

    private:
        //float moveSpeed_ = 150.0f;
        //float bounceForce_ = 200.0f;
        //float verticalBias_ = 0.3f; // How much to favor upward movement when hitting walls

        float accelX_ = 240.0f;   // horizontal acceleration  (units/s�)
        float accelYUp_ = 120.0f;   // vertical accel when moving UP toward target
        float accelYDown_ = 240.0f;   // vertical accel when coming DOWN
        float maxSpeedX_ = 240.0f;   // horizontal speed cap
        float maxSpeedY_ = 160.0f;   // vertical speed cap
        float hoverOffset_ = 75.0f;   // how far ABOVE the player to aim
        // Turnaround boost when velocity opposes target direction
        float turnaroundMult_ = 0.5f;
        Entity ownerID_ = 0;
    };

    // --- CHASE STATE (Pathfinding) ---
    class BirdChaseState : public NavState {
    public:
        const char* GetName() const override { return "Chase"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

    private:
        std::vector<Vector2D> currentPath_;
        float moveSpeed_ = 100.0f;
        float repathTimer_ = 0.0f;
        float repathInterval_ = 0.5f;
    };

    // --- BOUNCE STATE (After hitting wall) --- 
    class BirdBounceState : public State {
    public:
        const char* GetName() const override { return "Bounce"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetDuration() const { return bounceDuration_; }

    private:
        float timer_ = 0.0f;
        float bounceDuration_ = 0.5f;
        float bounceFactor_ = 1.f;    // energy retained after bounce (Demon Eye = 0.6)
        float nudgeAmount_ = 3.0f;    // pixels to push out of wall on enter
        // Store the collision normal so we know which way to nudge
        float collisionNormalX_ = 0.0f;  // -1 or +1, set in OnEnter
        Vector2D bounceDirection_;
        Entity ownerID_ = 0;
    };

    // --- HOVER STATE ---
    class BirdHoverState : public State {
    public:
        const char* GetName() const override { return "Hover"; }
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetAttackTimer() const { return attackTimer_; }

    private:
        float hoverRadius_ = 200.0f;
        float hoverSpeed_ = 50.0f;
        float attackTimer_ = 0.0f;
        float attackCooldown_ = 3.0f; // Time before it can attack
    };

    // --- ATTACK STATE (Dive) ---
    class BirdAttackState : public State {
    public:
        const char* GetName() const override { return "Attack"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;
        float GetTimer() const { return timer_; }
        float GetDuration() const { return duration_; }

    private:
        Vector2D direction_;
        float attackAccel_ = 600.0f; // acceleration along dive direction
        float attackMaxSpeed_ = 400.0f; // speed cap during dive
        float duration_ = 2.0f;
        float timer_ = 0.0f;
        Entity ownerID_ = 0;
    };

    // --- RECOVERY STATE ---
    class BirdRecoveryState : public State {
    public:
        const char* GetName() const override { return "Recovery"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetDuration() const { return duration_; }
        float GetTimer() const { return timer_; }

    private:
        float duration_ = 1.f;
        float timer_ = 0.0f;
        Entity ownerID_ = 0;
    };


    // --- DEATH STATE ---
    class BirdDeathState : public State {
    public:
        const char* GetName() const override { return "Death"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetDuration() const { return duration_; }

    private:
        float timer_ = 0.0f;
        float duration_ = 0.9f;
    };

    // ========================================
    // WITCH ENEMY STATES
    // ========================================

    // --- WITCH IDLE STATE ---
    class WitchIdleState : public State {
    public:
        const char* GetName() const override { return "WitchIdle"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

    private:
        float detectionRadius_ = 400.0f;
    };

    // --- WITCH ALERTED STATE ---
    class WitchAlertedState : public State {
    public:
        const char* GetName() const override { return "WitchAlerted"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetAlertDuration() const { return alertDuration_; }

    private:
        float timer_ = 0.0f;
        float alertDuration_ = 1.0f;
    };

    // --- WITCH ENRAGED STATE ---
    class WitchEnragedState : public State {
    public:
        const char* GetName() const override { return "WitchEnraged"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetEnrageDuration() const { return enrageDuration_; }

    private:
        float timer_ = 0.0f;
        float enrageDuration_ = 0.5f;
    };

    // --- WITCH TELEPORT STATE ---
    class WitchTeleportState : public State {
    public:
        const char* GetName() const override { return "WitchTeleport"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetTeleportDuration() const { return teleportDuration_; }

    private:
        float timer_ = 0.0f;
        float teleportDuration_ = 0.3f;
        float minTeleportRange_ = 80.0f;
        float maxTeleportRange_ = 200.0f;
    };

    // --- WITCH CHARGING STATE ---
    class WitchChargingState : public State {
    public:
        const char* GetName() const override { return "WitchCharging"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetChargeDuration() const { return chargeDuration_; }
        float GetAttackRadius() const { return attackRadius_; }

    private:
        float timer_ = 0.0f;
        float chargeDuration_ = 1.5f;
        float attackRadius_ = 150.0f;
    };

    // --- WITCH ATTACK STATE ---
    class WitchAttackState : public State {
    public:
        const char* GetName() const override { return "WitchAttack"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void OnExit(Entity entity, EntityStorage& ecs) override;
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetAttackDuration() const { return attackDuration_; }
        float GetAttackRadius() const { return maxAttackRadius_; }
        float GetCurrentRadius() const { return currentRadius_; }

    private:
        float timer_ = 0.0f;
        float attackDuration_ = 2.0f;
        float maxAttackRadius_ = 200.0f;
        float currentRadius_ = 0.0f;
        bool damageDealt_ = false;
    };
}