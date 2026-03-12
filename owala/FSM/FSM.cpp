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

    // Registers a new state. Silently ignored if a state with this name already exists.
    void FSM::AddState(const std::string& name, std::shared_ptr<State> state) {
        if (states_.find(name) != states_.end()) return;
        states_[name] = state;
    }

    // -----------------------------------------------------------------------

    // Registers a conditional transition from one state to another.
    // Both states must already exist or the transition is silently ignored.
    void FSM::AddTransition(const std::string& from, const std::string& to, std::function<bool(Entity, EntityStorage&)> cond) {
        if (states_.find(from) == states_.end() || states_.find(to) == states_.end()) {
            // Log error: invalid transition
            return;
        }
        transitions_[from].push_back(std::make_shared<Transition>(to, cond));
    }

    // -----------------------------------------------------------------------

    // Sets which state the FSM will enter when Initialize() is called.
    void FSM::SetInitialState(const std::string& name) {
        if (states_.find(name) != states_.end()) {
            initialState_ = name;
        }
    }

    // -----------------------------------------------------------------------

    // Enters the initial state, calling its OnEnter(). Called once when an entity is first set up.
    void FSM::Initialize(Entity entity, EntityStorage& ecs) {
        if (initialState_.empty()) return;
        currentState_ = initialState_;
        auto it = states_.find(currentState_);
        if (it != states_.end() && it->second) {
            it->second->OnEnter(entity, ecs);
        }
    }

    // -----------------------------------------------------------------------

    // Each frame: checks transitions from the current state, switches if a condition fires,
    // then calls OnUpdate() on the active state if no transition occurred.
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

    // Immediately switches to a new state: calls OnExit on the current state,
    // stores it as previousState_, then calls OnEnter on the new state.
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

    // Returns a raw pointer to a registered state by name, or nullptr if not found.
    State* FSM::GetState(const std::string& name) {
        auto it = states_.find(name);
        return (it != states_.end()) ? it->second.get() : nullptr;
    }

} // namespace Framework