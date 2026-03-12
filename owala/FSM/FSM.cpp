/*********************************************************************//**
 * \file   FSM.cpp
 *
 * \primary_author
 *      Lur Pei Hao Stanley (100%)
 *
 * \brief
 *      Implements the runtime logic for the Finite State Machine, managing
 *      state switching, transition checks, and lifecycle execution
 *      (OnEnter, OnUpdate, OnExit).
 *
 * \details
 *      This file contains the implementation of the Finite State Machine (FSM)
 *      system. It provides a flexible state machine framework that can be
 *      attached to entities to manage AI behavior, game logic, or any other
 *      state-based systems. Key features include:
 *        - State registration and management
 *        - Transition definitions with custom condition checks
 *        - Automatic state switching based on transitions
 *        - Lifecycle callbacks (OnEnter, OnUpdate, OnExit)
 *        - History tracking of previous state
 *
 *      Example Usage:
 *      \code
 *      // Create states
 *      auto idleState = std::make_shared<IdleState>();
 *      auto chaseState = std::make_shared<ChaseState>();
 *
 *      // Configure FSM
 *      FSM fsm;
 *      fsm.AddState("Idle", idleState);
 *      fsm.AddState("Chase", chaseState);
 *      fsm.AddTransition("Idle", "Chase", [](Entity e, EntityStorage& ecs) {
 *          // Check if player is in range
 *          return IsPlayerInRange(e, ecs);
 *      });
 *      fsm.SetInitialState("Idle");
 *
 *      // Use in entity update
 *      fsm.Initialize(entity, ecs);
 *      fsm.Update(entity, ecs, deltaTime);
 *      \endcode
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the
 *      prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "FSM.h"
#include "Logger.h" 

namespace Framework {

    // -----------------------------------------------------------------------

    /**
     * @brief Adds a state to the state machine.
     *
     * Registers a new state with the given name. If a state with this name
     * already exists, the operation is silently ignored.
     *
     * @param name Unique identifier for the state
     * @param state Shared pointer to the State implementation
     */
    void FSM::AddState(const std::string& name, std::shared_ptr<State> state) {
        if (states_.find(name) != states_.end()) return;
        states_[name] = state;
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Adds a transition between two states.
     *
     * Defines a condition that, when true, will trigger a switch from the
     * 'from' state to the 'to' state. Transitions are checked each frame
     * during Update() before the current state's OnUpdate is called.
     *
     * @param from Name of the source state
     * @param to Name of the destination state
     * @param cond Condition function that returns true when transition should occur
     *
     * @note If either state doesn't exist, the transition is silently ignored.
     */
    void FSM::AddTransition(const std::string& from, const std::string& to, std::function<bool(Entity, EntityStorage&)> cond) {
        if (states_.find(from) == states_.end() || states_.find(to) == states_.end()) {
            // Log error: invalid transition
            return;
        }
        transitions_[from].push_back(std::make_shared<Transition>(to, cond));
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Sets the initial state for the state machine.
     *
     * The state machine will start in this state when Initialize() is called.
     *
     * @param name Name of the initial state
     */
    void FSM::SetInitialState(const std::string& name) {
        if (states_.find(name) != states_.end()) {
            initialState_ = name;
        }
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Initializes the state machine, entering the initial state.
     *
     * Should be called once when the entity is first created or when the
     * state machine is first attached. Calls OnEnter on the initial state.
     *
     * @param entity The entity this FSM is attached to
     * @param ecs Reference to the entity storage for component access
     */
    void FSM::Initialize(Entity entity, EntityStorage& ecs) {
        if (initialState_.empty()) return;
        currentState_ = initialState_;
        auto it = states_.find(currentState_);
        if (it != states_.end() && it->second) {
            it->second->OnEnter(entity, ecs);
        }
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Updates the state machine, checking transitions and updating current state.
     *
     * Called every frame to:
     * 1. Check all transitions from the current state
     * 2. If any transition condition is true, switch to the target state
     * 3. If no transition occurs, update the current state
     *
     * @param entity The entity this FSM is attached to
     * @param ecs Reference to the entity storage for component access
     * @param dt Delta time since last frame
     */
    void FSM::Update(Entity entity, EntityStorage& ecs, float dt) {
        if (currentState_.empty()) return;

        // Check Transitions
        if (transitions_.find(currentState_) != transitions_.end()) {
            for (const auto& trans : transitions_[currentState_]) {
                if (trans->Check(entity, ecs)) {
                    ChangeState(entity, ecs, trans->GetTargetState());
                    return; // Exit update immediately on state change
                }
            }
        }

        // Update Current State
        auto it = states_.find(currentState_);
        if (it != states_.end() && it->second) {
            it->second->OnUpdate(entity, ecs, dt);
        }
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Changes to a new state, handling exit and enter callbacks.
     *
     * Internal method that:
     * 1. Calls OnExit on the current state
     * 2. Stores the previous state
     * 3. Switches to the new state
     * 4. Calls OnEnter on the new state
     *
     * @param entity The entity this FSM is attached to
     * @param ecs Reference to the entity storage
     * @param newState Name of the state to switch to
     */
    void FSM::ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState) {
        if (newState == currentState_) return;
        if (states_.find(newState) == states_.end()) return;

        auto curIt = states_.find(currentState_);
        if (curIt != states_.end() && curIt->second) curIt->second->OnExit(entity, ecs);

        previousState_ = currentState_;
        currentState_ = newState;

        auto newIt = states_.find(currentState_);
        if (newIt != states_.end() && newIt->second) newIt->second->OnEnter(entity, ecs);
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Retrieves a pointer to a state by name.
     *
     * @param name Name of the state to retrieve
     * @return State* Pointer to the state, or nullptr if not found
     */
    State* FSM::GetState(const std::string& name) {
        auto it = states_.find(name);
        return (it != states_.end()) ? it->second.get() : nullptr;
    }

} // namespace Framework