#pragma once
/*****************************************************************//**
* \file   FSM.h
*
* \primary_author
*      [Your Name]
*
* \brief
*   Generic Finite State Machine implementation that can be configured
*   from JSON data. Supports states, transitions, and conditions.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
*********************************************************************/
#pragma once
#include "Entity.h"
#include "EntityStorage.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Framework {

    // Forward declarations
    class FSM;
    class State;
    class Transition;

    /**
     * @class State
     * @brief Base class for FSM states. Override to create custom behaviors.
     */
    class State {
    public:
        virtual ~State() = default;

        // Called once when entering this state
        virtual void OnEnter(Entity entity, EntityStorage& ecs) {}

        // Called every frame while in this state
        virtual void OnUpdate(Entity entity, EntityStorage& ecs, float dt) {}

        // Called once when exiting this state
        virtual void OnExit(Entity entity, EntityStorage& ecs) {}

        // Returns the name of this state
        virtual const char* GetName() const = 0;

        // Set parameters from JSON (override if your state needs config)
        virtual void SetParameter(const std::string& key, float value) {}
        virtual void SetParameter(const std::string& key, const std::string& value) {}
    };

    /**
     * @class Transition
     * @brief Represents a transition between states with a condition.
     */
    class Transition {
    private:
        std::string targetState_;
        std::function<bool(Entity, EntityStorage&)> condition_;

    public:
        Transition(const std::string& targetState,
            std::function<bool(Entity, EntityStorage&)> condition)
            : targetState_(targetState), condition_(condition) {}

        bool CheckCondition(Entity entity, EntityStorage& ecs) const {
            return condition_(entity, ecs);
        }

        const std::string& GetTargetState() const { return targetState_; }
    };

    /**
     * @class FSM
     * @brief Generic Finite State Machine that manages states and transitions.
     */
    class FSM {
    private:
        std::unordered_map<std::string, std::unique_ptr<State>> states_;
        std::unordered_map<std::string, std::vector<std::unique_ptr<Transition>>> transitions_;

        std::string initialState_;
        std::string currentState_;
        std::string previousState_;

    public:
        FSM() = default;

        // Add a state to the FSM
        void AddState(const std::string& name, std::unique_ptr<State> state);

        // Add a transition from one state to another
        void AddTransition(const std::string& fromState,
            const std::string& toState,
            std::function<bool(Entity, EntityStorage&)> condition);

        // Set the initial state
        void SetInitialState(const std::string& stateName);

        // Initialize the FSM (enter initial state)
        void Initialize(Entity entity, EntityStorage& ecs);

        // Update the FSM (check transitions and update current state)
        void Update(Entity entity, EntityStorage& ecs, float dt);

        // Force a state change
        void ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState);

        // Getters
        const std::string& GetCurrentState() const { return currentState_; }
        const std::string& GetPreviousState() const { return previousState_; }
        bool IsInState(const std::string& stateName) const { return currentState_ == stateName; }

        // Get a state by name (for configuration)
        State* GetState(const std::string& name);
    };

} // namespace Framework