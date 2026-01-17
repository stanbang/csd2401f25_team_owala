/*****************************************************************//**
* \file   FSM.cpp
*
* \brief  Implementation of the generic FSM system.
*********************************************************************/
#include "FSM.h"
#include "Logger.h"
#include "EntityStorage.h"

namespace Framework {

    void FSM::AddState(const std::string& name, std::shared_ptr<State> state) {
        if (states_.find(name) != states_.end()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: State '" << name << "' already exists!" << std::endl;
            return;
        }
        states_[name] = state; 

    }

    void FSM::AddTransition(const std::string& fromState,
        const std::string& toState,
        std::function<bool(Entity, EntityStorage&)> condition) {
        if (states_.find(fromState) == states_.end()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: Cannot add transition from non-existent state '"
                << fromState << "'" << std::endl;
            return;
        }

        if (states_.find(toState) == states_.end()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: Cannot add transition to non-existent state '"
                << toState << "'" << std::endl;
            return;
        }

        transitions_[fromState].push_back(
            std::make_shared<Transition>(toState, condition)
        );
    }

    void FSM::SetInitialState(const std::string& stateName) {
        if (states_.find(stateName) == states_.end()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: Cannot set non-existent state '"
                << stateName << "' as initial state!" << std::endl;
            return;
        }
        initialState_ = stateName;
    }

    void FSM::Initialize(Entity entity, EntityStorage& ecs) {
        if (initialState_.empty()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: No initial state set!" << std::endl;
            return;
        }

        currentState_ = initialState_;
        previousState_ = "";

        if (states_[currentState_]) {
            states_[currentState_]->OnEnter(entity, ecs);
        }
    }

    void FSM::Update(Entity entity, EntityStorage& ecs, float dt) {
        if (currentState_.empty()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: No current state!" << std::endl;
            return;
        }

        // Check for transitions
        if (transitions_.find(currentState_) != transitions_.end()) {
            for (const auto& transition : transitions_[currentState_]) {
                if (transition->CheckCondition(entity, ecs)) {
                    // Transition triggered!
                    ChangeState(entity, ecs, transition->GetTargetState());
                    break; // Only take the first valid transition
                }
            }
        }

        // Update current state
        if (states_[currentState_]) {
            states_[currentState_]->OnUpdate(entity, ecs, dt);
        }
    }

    void FSM::ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState) {
        if (newState == currentState_) return; // Already in this state

        if (states_.find(newState) == states_.end()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSM: Cannot change to non-existent state '" << newState << "'" << std::endl;
            return;
        }

        // Exit old state
        if (states_[currentState_]) {
            states_[currentState_]->OnExit(entity, ecs);
        }

        previousState_ = currentState_;
        currentState_ = newState;

        // Enter new state
        if (states_[currentState_]) {
            states_[currentState_]->OnEnter(entity, ecs);
        }

        Logger::Get().Info(DebugType::AISystem)
            << "FSM: State changed from '" << previousState_
            << "' to '" << currentState_ << "'" << std::endl;
    }

    State* FSM::GetState(const std::string& name) {
        auto it = states_.find(name);
        if (it != states_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

} // namespace Framework