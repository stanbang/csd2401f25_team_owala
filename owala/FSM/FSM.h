/*********************************************************************//**
* \file   FSM.h
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Defines the core FSM class and State/Transition base classes used to
* construct and manage behavioral graphs for entities.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Entity.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Framework {

    class FSM;
    class EntityStorage;
    class NavigationGrid; // Forward declare

    /**
     * @class State
     * @brief Abstract base class for all AI States.
     */
    class State {
    public:
        virtual ~State() = default;

        // Lifecycle hooks
        virtual void OnEnter(Entity entity, EntityStorage& ecs) { (void)entity; (void)ecs; }
        virtual void OnUpdate(Entity entity, EntityStorage& ecs, float dt) { (void)entity; (void)ecs; (void)dt; }
        virtual void OnExit(Entity entity, EntityStorage& ecs) { (void)entity; (void)ecs; }

        // Configuration
        virtual const char* GetName() const = 0;
        virtual void SetParameter(const std::string& key, float value) { (void)key; (void)value; }
        virtual void SetParameter(const std::string& key, const std::string& value) { (void)key; (void)value; }

        // Dependency Injection 
        virtual void SetNavGrid(NavigationGrid* grid) { (void)grid; }
    };

    /**
     * @class Transition
     * @brief Defines a link between two states guarded by a condition.
     */
    class Transition {
    private:
        std::string targetState_;
        std::function<bool(Entity, EntityStorage&)> condition_;

    public:
        Transition(const std::string& target, std::function<bool(Entity, EntityStorage&)> cond)
            : targetState_(target), condition_(cond) {}

        bool Check(Entity entity, EntityStorage& ecs) const {
            return condition_ ? condition_(entity, ecs) : false;
        }

        const std::string& GetTargetState() const { return targetState_; }
    };

    /**
     * @class FSM
     * @brief The runtime machine attached to an entity.
     */
    class FSM {
    private:
        std::unordered_map<std::string, std::shared_ptr<State>> states_;
        std::unordered_map<std::string, std::vector<std::shared_ptr<Transition>>> transitions_;

        std::string initialState_;
        std::string currentState_;
        std::string previousState_;

    public:
        void AddState(const std::string& name, std::shared_ptr<State> state);
        void AddTransition(const std::string& from, const std::string& to, std::function<bool(Entity, EntityStorage&)> cond);
        void SetInitialState(const std::string& name);

        void Initialize(Entity entity, EntityStorage& ecs);
        void Update(Entity entity, EntityStorage& ecs, float dt);
        void ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState);

        const std::string& GetCurrentState() const { return currentState_; }
        State* GetState(const std::string& name);
    };
}