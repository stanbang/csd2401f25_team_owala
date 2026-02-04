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

        float accelX_ = 240.0f;   // horizontal acceleration  (units/s²)
        float accelYUp_ = 120.0f;   // vertical accel when moving UP toward target
        float accelYDown_ = 240.0f;   // vertical accel when swooping DOWN
        float maxSpeedX_ = 240.0f;   // horizontal speed cap
        float maxSpeedY_ = 160.0f;   // vertical speed cap
        float hoverOffset_ = 75.0f;   // how far ABOVE the player to aim
        // Turnaround boost multiplier when velocity opposes target direction
        float turnaroundMult_ = 0.5f;
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

    // --- BOUNCE STATE (After hitting wall) --- previous hover state
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
}