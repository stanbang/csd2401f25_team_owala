/*****************************************************************//**
 * \file   AIStates.h
 * \brief  Concrete state definitions for Bird and generic AI.
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
        float wakeUpTime_ = 1.0f;
    };

    // --- SIMPLE CHASE STATE (No Pathfinding) ---
    class BirdSimpleChaseState : public State {
    public:
        const char* GetName() const override { return "SimpleChase"; }
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void SetParameter(const std::string& key, float value) override;

    private:
        float moveSpeed_ = 150.0f;
        float bounceForce_ = 200.0f;
        float verticalBias_ = 0.3f; // How much to favor upward movement when hitting walls
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
        float bounceDuration_ = 0.3f;
        Vector2D bounceDirection_;
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
        float attackSpeed_ = 400.0f;
        float duration_ = 2.0f;
        float timer_ = 0.0f;
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
        float duration_ = 1.5f;
        float timer_ = 0.0f;
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
        float teleportDistance_ = 200.0f;
        Vector2D targetPosition_;
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
        void SetParameter(const std::string& key, float value) override;

        float GetTimer() const { return timer_; }
        float GetAttackDuration() const { return attackDuration_; }
        float GetAttackRadius() const { return attackRadius_; }

    private:
        float timer_ = 0.0f;
        float attackDuration_ = 0.5f;
        float attackRadius_ = 150.0f;
        bool damageDealt_ = false;
    };
}