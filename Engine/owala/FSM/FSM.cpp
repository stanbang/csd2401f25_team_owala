#include "FSM.h"
#include "Logger.h" 

namespace Framework {

    void FSM::AddState(const std::string& name, std::shared_ptr<State> state) {
        if (states_.find(name) != states_.end()) return;
        states_[name] = state;
    }

    void FSM::AddTransition(const std::string& from, const std::string& to, std::function<bool(Entity, EntityStorage&)> cond) {
        if (states_.find(from) == states_.end() || states_.find(to) == states_.end()) {
            // Log error: invalid transition
            return;
        }
        transitions_[from].push_back(std::make_shared<Transition>(to, cond));
    }

    void FSM::SetInitialState(const std::string& name) {
        if (states_.find(name) != states_.end()) {
            initialState_ = name;
        }
    }

    void FSM::Initialize(Entity entity, EntityStorage& ecs) {
        if (initialState_.empty()) return;
        currentState_ = initialState_;
        if (states_[currentState_]) {
            states_[currentState_]->OnEnter(entity, ecs);
        }
    }

    void FSM::Update(Entity entity, EntityStorage& ecs, float dt) {
        if (currentState_.empty()) return;

        // 1. Check Transitions
        if (transitions_.find(currentState_) != transitions_.end()) {
            for (const auto& trans : transitions_[currentState_]) {
                if (trans->Check(entity, ecs)) {
                    ChangeState(entity, ecs, trans->GetTargetState());
                    return; // Exit update immediately on state change
                }
            }
        }

        // 2. Update Current State
        if (states_[currentState_]) {
            states_[currentState_]->OnUpdate(entity, ecs, dt);
        }
    }

    void FSM::ChangeState(Entity entity, EntityStorage& ecs, const std::string& newState) {
        if (newState == currentState_) return;
        if (states_.find(newState) == states_.end()) return;

        if (states_[currentState_]) states_[currentState_]->OnExit(entity, ecs);

        previousState_ = currentState_;
        currentState_ = newState;

        if (states_[currentState_]) states_[currentState_]->OnEnter(entity, ecs);
    }

    State* FSM::GetState(const std::string& name) {
        auto it = states_.find(name);
        return (it != states_.end()) ? it->second.get() : nullptr;
    }
}