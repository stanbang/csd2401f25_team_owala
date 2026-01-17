#pragma once
/*****************************************************************//**
* \file  AIStates.h
*
* \brief  Concrete state implementations for Bird AI.
*********************************************************************/
#pragma once
#include "FSM.h"
#include "Pathfinding.h"
#include "EntityStorage.h"

namespace Framework {

    /**
     * @class BirdIdleState
     * @brief Bird is sleeping, waiting for player to come close.
     */
    class BirdIdleState : public State {
    private:
        float wakeUpRadius_ = 288.0f;

    public:
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        const char* GetName() const override { return "Idle"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "wakeUpRadius") wakeUpRadius_ = value;
        }

        float GetWakeUpRadius() const { return wakeUpRadius_; }
    };

    /**
     * @class BirdWakingState
     * @brief Bird is waking up (plays animation, waits for timer).
     */
    class BirdWakingState : public State {
    private:
        float wakeUpTime_ = 1.0f;
        float timer_ = 0.0f;

    public:
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void OnExit(Entity entity, EntityStorage& ecs) override;
        const char* GetName() const override { return "Waking"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "wakeUpTime") wakeUpTime_ = value;
        }

        float GetTimer() const { return timer_; }
        float GetWakeUpTime() const { return wakeUpTime_; }
    };

    /**
     * @class BirdChaseState
     * @brief Bird uses pathfinding to chase the player.
     */
    class BirdChaseState : public State {
    private:
        float moveSpeed_ = 200.0f;
        float hoverRadius_ = 48.0f;
        float pathRecalculateInterval_ = 0.5f;
        float pathRecalculateTimer_ = 0.0f;

        std::vector<Vector2D> currentPath_;
        NavigationGrid* navGrid_ = nullptr;
        AStarPather pather_;

    public:
        void SetNavGrid(NavigationGrid* grid) { navGrid_ = grid; }

        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void OnExit(Entity entity, EntityStorage& ecs) override;
        const char* GetName() const override { return "Chase"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "moveSpeed") moveSpeed_ = value;
            else if (key == "hoverRadius") hoverRadius_ = value;
            else if (key == "pathRecalculateInterval") pathRecalculateInterval_ = value;
        }

        const std::vector<Vector2D>& GetCurrentPath() const { return currentPath_; }
    };

    /**
     * @class BirdHoverState
     * @brief Bird hovers around the player in a patrol pattern.
     */
    class BirdHoverState : public State {
    private:
        float hoverHeight_ = 648.0f;
        float hoverRadius_ = 324.0f;
        float hoverSpeed_ = 162.0f;
        float hoverNodeReachedDist_ = 54.0f;
        float attackRadius_ = 96.0f;
        float attackCooldown_ = 3.0f;

        int currentHoverNode_ = 0;
        float attackTimer_ = 0.0f;

    public:
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        const char* GetName() const override { return "Hover"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "hoverHeight") hoverHeight_ = value;
            else if (key == "hoverRadius") hoverRadius_ = value;
            else if (key == "hoverSpeed") hoverSpeed_ = value;
            else if (key == "hoverNodeReachedDist") hoverNodeReachedDist_ = value;
            else if (key == "attackRadius") attackRadius_ = value;
            else if (key == "attackCooldown") attackCooldown_ = value;
        }

        float GetAttackTimer() const { return attackTimer_; }
    };

    /**
     * @class BirdAttackState
     * @brief Bird dives at the player's last known position.
     */
    class BirdAttackState : public State {
    private:
        float attackSpeed_ = 300.0f;
       /* float attackDuration_ = 1.0f;
        float timer_ = 0.0f;
        Vector2D attackTargetPosition_;*/
        Vector2D diveDirection_ = { 0.0f, 0.0f }; // Fixed direction

    public:
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        void OnExit(Entity entity, EntityStorage& ecs) override;
        const char* GetName() const override { return "Attack"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "attackSpeed") attackSpeed_ = value;
           // else if (key == "attackDuration") attackDuration_ = value;
        }
    };

    /**
     * @class BirdRecoveryState
     * @brief A buffer state where the bird is stunned/recovering after hitting a wall.
     */
    class BirdRecoveryState : public State {
    private:
        float recoveryDuration_ = 2.0f;
        float timer_ = 0.0f;

    public:
        void OnEnter(Entity entity, EntityStorage& ecs) override;
        void OnUpdate(Entity entity, EntityStorage& ecs, float dt) override;
        const char* GetName() const override { return "Recovery"; }

        void SetParameter(const std::string& key, float value) override {
            if (key == "recoveryDuration") recoveryDuration_ = value;
        }

        float GetTimer() const { return timer_; }
        float GetDuration() const { return recoveryDuration_; }
    };

} // namespace Framework