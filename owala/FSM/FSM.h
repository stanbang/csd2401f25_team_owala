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
     * \class State
     * \brief Abstract base class for all FSM AI states.
     *
     * Provides the lifecycle interface (OnEnter, OnUpdate, OnExit) that every
     * concrete state must implement. Default no-op implementations are provided
     * so derived classes only need to override what they actually use.
     */
    class State {
    public:
        /** \brief Virtual destructor ensuring safe polymorphic deletion. */
        virtual ~State() = default;

        /**
         * \brief Called once when the FSM transitions into this state.
         * \param entity The entity whose FSM is entering this state.
         * \param ecs    Reference to the ECS storage for component access.
         */
        virtual void OnEnter(Entity entity, EntityStorage& ecs) { (void)entity; (void)ecs; }

        /**
         * \brief Called every frame while this state is active.
         * \param entity The entity whose FSM is running this state.
         * \param ecs    Reference to the ECS storage for component access.
         * \param dt     Delta time in seconds since the last frame.
         */
        virtual void OnUpdate(Entity entity, EntityStorage& ecs, float dt) { (void)entity; (void)ecs; (void)dt; }

        /**
         * \brief Called once when the FSM transitions out of this state.
         * \param entity The entity whose FSM is leaving this state.
         * \param ecs    Reference to the ECS storage for component access.
         */
        virtual void OnExit(Entity entity, EntityStorage& ecs) { (void)entity; (void)ecs; }

        /**
         * \brief Returns the unique string identifier for this state.
         * \return Null-terminated string name used for lookup and logging.
         */
        virtual const char* GetName() const = 0;

        /**
         * \brief Sets a named float configuration parameter on this state.
         * \param key   Parameter name (e.g. "detectionRadius").
         * \param value Float value to assign.
         */
        virtual void SetParameter(const std::string& key, float value) { (void)key; (void)value; }

        /**
         * \brief Sets a named string configuration parameter on this state.
         * \param key   Parameter name.
         * \param value String value to assign.
         */
        virtual void SetParameter(const std::string& key, const std::string& value) { (void)key; (void)value; }

        /**
         * \brief Returns whether this state has naturally completed.
         *
         * Override in terminal states (e.g. death animations) so the FSM or
         * external systems can react without needing knowledge of derived types.
         * \return true once the state considers itself finished; false by default.
         */
        virtual bool IsFinished() const { return false; }

        /**
         * \brief Injects a NavigationGrid dependency for pathfinding states.
         * \param grid Pointer to the navigation grid; may be nullptr.
         */
        virtual void SetNavGrid(NavigationGrid* grid) { (void)grid; }
    };

    /**
     * \class Transition
     * \brief Defines a guarded link between two FSM states.
     *
     * Each transition holds a target state name and a condition predicate.
     * The FSM evaluates Check() every frame on all transitions leaving the
     * active state; the first one that returns true triggers a state change.
     */
    class Transition {
    private:
        std::string targetState_;
        std::function<bool(Entity, EntityStorage&)> condition_;

    public:
        /**
         * \brief Constructs a transition to a target state with a condition.
         * \param target Name of the state to transition into when condition is met.
         * \param cond   Predicate evaluated each frame; returns true to fire.
         */
        Transition(const std::string& target, std::function<bool(Entity, EntityStorage&)> cond)
            : targetState_(target), condition_(cond) {}

        /**
         * \brief Evaluates whether this transition should fire.
         * \param entity The entity owning the FSM.
         * \param ecs    ECS storage for component queries.
         * \return true if the condition is satisfied and the FSM should transition.
         */
        bool Check(Entity entity, EntityStorage& ecs) const {
            return condition_ ? condition_(entity, ecs) : false;
        }

        /**
         * \brief Returns the name of the target state for this transition.
         * \return Reference to the target state name string.
         */
        const std::string& GetTargetState() const { return targetState_; }
    };

    /**
     * \class FSM
     * \brief Runtime finite state machine attached to an entity.
     *
     * Stores a map of named State objects and a list of Transition rules.
     * Each frame, Update() evaluates all transitions from the current state
     * and calls the active state's OnUpdate(). State changes invoke OnExit()
     * on the leaving state and OnEnter() on the entering state.
     */
    class FSM {
    private:
        std::unordered_map<std::string, std::shared_ptr<State>> states_;
        std::unordered_map<std::string, std::vector<std::shared_ptr<Transition>>> transitions_;

        std::string initialState_;
        std::string currentState_;
        std::string previousState_;

    public:
        /**
         * \brief Registers a state with the FSM under the given name.
         * \param name  Unique string identifier for this state.
         * \param state Shared pointer to the State instance.
         */
        void AddState(const std::string& name, std::shared_ptr<State> state);

        /**
         * \brief Registers a conditional transition between two states.
         * \param from Name of the source state.
         * \param to   Name of the target state.
         * \param cond Predicate that returns true when the transition should fire.
         */
        void AddTransition(const std::string& from, const std::string& to, std::function<bool(Entity, EntityStorage&)> cond);

        /**
         * \brief Sets the state the FSM will enter when Initialize() is called.
         * \param name Name of the initial state.
         */
        void SetInitialState(const std::string& name);

        /**
         * \brief Enters the initial state, invoking its OnEnter() callback.
         * \param entity The entity that owns this FSM.
         * \param ecs    ECS storage for component access.
         */
        void Initialize(Entity entity, EntityStorage& ecs);

        /**
         * \brief Ticks the FSM: evaluates transitions then calls OnUpdate() on the active state.
         * \param entity The entity that owns this FSM.
         * \param ecs    ECS storage for component access.
         * \param dt     Delta time in seconds since the last frame.
         */
        void Update(Entity entity, EntityStorage& ecs, float dt);

        /**
         * \brief Forces an immediate transition to the named state.
         *
         * Calls OnExit() on the current state (if any) and OnEnter() on the new one.
         * \param entity   The entity that owns this FSM.
         * \param ecs      ECS storage for component access.
         * \param newState Name of the state to switch into.
         */
        void ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState);

        /**
         * \brief Returns the name of the currently active state.
         * \return Const reference to the current state name string.
         */
        const std::string& GetCurrentState() const { return currentState_; }

        /**
         * \brief Retrieves a raw pointer to a registered state by name.
         * \param name The state name to look up.
         * \return Pointer to the State, or nullptr if not found.
         */
        State* GetState(const std::string& name);
    };
}