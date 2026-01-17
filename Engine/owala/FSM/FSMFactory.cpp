/*****************************************************************//**
* \file   FSMFactory.cpp
*
* \brief  Implementation of FSM factory.
*********************************************************************/
#include "FSMFactory.h"
#include "AIStates.h"
#include "EntityManager.h"
#include "Logger.h"
#include <fstream>
#include <rapidjson/istreamwrapper.h>

namespace Framework {

    // Helper to get player
    static Entity GetPlayer(EntityStorage& ecs) {
        EntityManager mgr(ecs);
        auto players = mgr.GetAllEntitiesWithComponent(PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    FSM FSMFactory::CreateFromFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSMFactory: Failed to open file: " << filepath << std::endl;
            return FSM();
        }

        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document doc;
        doc.ParseStream(isw);

        if (doc.HasParseError()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSMFactory: JSON parse error in file: " << filepath << std::endl;
            return FSM();
        }

        return CreateFromJSON(doc);
    }

    FSM FSMFactory::CreateFromJSON(const rapidjson::Value& data) {
        FSM fsm;

        if (!data.IsObject()) {
            Logger::Get().Error(DebugType::AISystem)
                << "FSMFactory: Invalid JSON format" << std::endl;
            return fsm;
        }

        // Read initial state
        if (data.HasMember("initialState") && data["initialState"].IsString()) {
            fsm.SetInitialState(data["initialState"].GetString());
        }

        // Create states
        if (data.HasMember("states") && data["states"].IsArray()) {
            const rapidjson::Value& statesArray = data["states"];

            for (rapidjson::SizeType i = 0; i < statesArray.Size(); ++i) {
                const rapidjson::Value& stateData = statesArray[i];

                if (!stateData.HasMember("name") || !stateData["name"].IsString()) {
                    continue;
                }

                std::string stateName = stateData["name"].GetString();
                std::string stateType = stateData.HasMember("type") && stateData["type"].IsString()
                    ? stateData["type"].GetString()
                    : stateName;

                auto state = CreateState(stateType, stateData);
                if (state) {
                    fsm.AddState(stateName,state);
                }
            }
        }

        // Create transitions
        if (data.HasMember("transitions") && data["transitions"].IsArray()) {
            const rapidjson::Value& transitionsArray = data["transitions"];

            for (rapidjson::SizeType i = 0; i < transitionsArray.Size(); ++i) {
                const rapidjson::Value& transData = transitionsArray[i];

                if (!transData.HasMember("from") || !transData.HasMember("to") ||
                    !transData.HasMember("condition")) {
                    continue;
                }

                std::string fromState = transData["from"].GetString();
                std::string toState = transData["to"].GetString();

                const rapidjson::Value& conditionData = transData["condition"];
                std::string conditionType = conditionData["type"].GetString();

                auto condition = CreateCondition(conditionType, conditionData);
                if (condition) {
                    fsm.AddTransition(fromState, toState, condition);
                }
            }
        }

        return fsm;
    }

    std::shared_ptr<State> FSMFactory::CreateState(const std::string& stateName, const rapidjson::Value& stateData) {
        std::shared_ptr<State> state;

        // Create the appropriate state type
        if (stateName == "Idle") {
            state = std::make_shared<BirdIdleState>();
        }
        else if (stateName == "Waking") {
            state = std::make_shared<BirdWakingState>();
        }
        else if (stateName == "Chase") {
            auto chaseState = std::make_shared<BirdChaseState>();
            chaseState->SetNavGrid(navGrid_);
            state = chaseState;
        }
        else if (stateName == "Hover") {
            state = std::make_shared<BirdHoverState>();
        }
        else if (stateName == "Attack") {
            state = std::make_shared<BirdAttackState>();
        }
        else if (stateName == "Recovery") state = std::make_shared<BirdRecoveryState>(); 
        else {
            Logger::Get().Error(DebugType::AISystem)
                << "FSMFactory: Unknown state type: " << stateName << std::endl;
            return nullptr;
        }

        // Set parameters from JSON
        if (stateData.HasMember("parameters") && stateData["parameters"].IsObject()) {
            const rapidjson::Value& params = stateData["parameters"];

            for (auto it = params.MemberBegin(); it != params.MemberEnd(); ++it) {
                std::string key = it->name.GetString();

                if (it->value.IsNumber()) {
                    state->SetParameter(key, static_cast<float>(it->value.GetDouble()));
                }
                else if (it->value.IsString()) {
                    state->SetParameter(key, it->value.GetString());
                }
            }
        }

        return state;
    }

    std::function<bool(Entity, EntityStorage&)> FSMFactory::CreateCondition(
        const std::string& conditionType,
        const rapidjson::Value& conditionData) {

        if (conditionType == "PlayerInRange") {
            float radius = conditionData.HasMember("radius")
                ? static_cast<float>(conditionData["radius"].GetDouble())
                : 288.0f;

            return [radius](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);

                if (!mgr.HasComponent(entity, PositionType)) return false;

                Vector2D entityPos = mgr.GetComponent<Position>(entity).Pos;
                Entity player = GetPlayer(ecs);

                if (player == 0 || !mgr.HasComponent(player, PositionType)) return false;

                Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;
                return entityPos.Distance(playerPos) < radius;
                };
        }
        else if (conditionType == "TimerExpired") {
            std::string stateName = conditionData.HasMember("state")
                ? conditionData["state"].GetString()
                : "";

            return [stateName](Entity entity, EntityStorage& ecs) -> bool {
                // Access the FSM component to check the timer
                EntityManager mgr(ecs);
                if (!mgr.HasComponent(entity, FSMComponentType)) return false;

                FSMComponent& fsmComp = mgr.GetComponent<FSMComponent>(entity);
                State* state = fsmComp.stateMachine.GetState(stateName);
               

                if (auto* wakingState = dynamic_cast<BirdWakingState*>(state)) {
                    return wakingState->GetTimer() >= wakingState->GetWakeUpTime();
                }
                if (auto* recovery = dynamic_cast<BirdRecoveryState*>(state)) {
                    return recovery->GetTimer() >= recovery->GetDuration();
                }

                return false;
                };
        
        }
        else if (conditionType == "ReachedHoverDistance") {
            float hoverRadius = conditionData.HasMember("radius")
                ? static_cast<float>(conditionData["radius"].GetDouble())
                : 48.0f;

                return [hoverRadius](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);

                if (!mgr.HasComponent(entity, PositionType)) return false;

                Vector2D entityPos = mgr.GetComponent<Position>(entity).Pos;
                Entity player = GetPlayer(ecs);

                if (player == 0 || !mgr.HasComponent(player, PositionType)) return false;

                Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;

                // Get entity's collision size for accurate distance check
                Vector2D entityHalfSize = Vector2D::Zero();
                if (mgr.HasComponent(entity, TextureComponentType)) {
                    entityHalfSize = mgr.GetComponent<TextureComponent>(entity).HalfSize;
                }

                Vector2D playerHalfSize = Vector2D::Zero();
                if (mgr.HasComponent(player, TextureComponentType)) {
                    playerHalfSize = mgr.GetComponent<TextureComponent>(player).HalfSize;
                }

                float entityRadius = std::max(entityHalfSize.x, entityHalfSize.y);
                float playerRadius = std::max(playerHalfSize.x, playerHalfSize.y);
                float effectiveRadius = hoverRadius + entityRadius + playerRadius;

                return entityPos.Distance(playerPos) < effectiveRadius;
                };
        }
        else if (conditionType == "PlayerEscaped") {
            float escapeMultiplier = conditionData.HasMember("escapeMultiplier")
                ? static_cast<float>(conditionData["escapeMultiplier"].GetDouble())
                : 1.5f;

            float baseRadius = conditionData.HasMember("baseRadius")
                ? static_cast<float>(conditionData["baseRadius"].GetDouble())
                : 48.0f;

            return [escapeMultiplier, baseRadius](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);

                if (!mgr.HasComponent(entity, PositionType)) return false;

                Vector2D entityPos = mgr.GetComponent<Position>(entity).Pos;
                Entity player = GetPlayer(ecs);

                if (player == 0 || !mgr.HasComponent(player, PositionType)) return false;

                Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;
                float escapeDistance = baseRadius * escapeMultiplier;

                return entityPos.Distance(playerPos) > escapeDistance;
                };
        }
        else if (conditionType == "AttackReady") {
            return [](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);
                if (!mgr.HasComponent(entity, FSMComponentType)) return false;

                FSMComponent& fsmComp = mgr.GetComponent<FSMComponent>(entity);
                State* state = fsmComp.stateMachine.GetState("Hover");

                if (auto* hoverState = dynamic_cast<BirdHoverState*>(state)) {
                    return hoverState->GetAttackTimer() <= 0.0f;
                }
                return false;
                };
        }
        else if (conditionType == "AttackComplete") {
            float attackDuration = conditionData.HasMember("duration")
                ? static_cast<float>(conditionData["duration"].GetDouble())
                : 1.0f;

            return [attackDuration](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);

                if (!mgr.HasComponent(entity, PositionType)) return false;

                // Check if attack duration has elapsed OR if close to target
                Entity player = GetPlayer(ecs);
                if (player != 0 && mgr.HasComponent(player, PositionType)) {
                    Vector2D entityPos = mgr.GetComponent<Position>(entity).Pos;
                    Vector2D playerPos = mgr.GetComponent<Position>(player).Pos;

                    // Attack is complete if we're very close to target
                    if (entityPos.Distance(playerPos) < 20.0f) {
                        return true;
                    }
                }

                // Otherwise check timer (would need to store this in FSMComponent)
                // For now, we'll rely on the distance check
                return false;
                };


        }

        else if (conditionType == "HasHitObstacle") {
            return [](Entity entity, EntityStorage& ecs) -> bool {
                EntityManager mgr(ecs);
                if (!mgr.HasComponent(entity, VelocityType)) return true;

                Vector2D vel = mgr.GetComponent<Velocity>(entity).Vel;

                // If velocity is almost zero (Physics stopped it against a wall)
                // But we know the Attack State is applying 500 force.
                return (std::abs(vel.x) < 10.0f && std::abs(vel.y) < 10.0f);
                };
        }

        Logger::Get().Error(DebugType::AISystem)
            << "FSMFactory: Unknown condition type: " << conditionType << std::endl;
        return nullptr;
    }

} // namespace Framework