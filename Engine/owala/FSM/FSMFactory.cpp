/*********************************************************************//**
* \file   FSMFactory.cpp
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Implements the data-driven creation of FSMs, parsing JSON configurations
* to dynamically instantiate states and conditions using a static registry.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "FSMFactory.h"
#include "FileUtility.h" 
#include "Logger.h"

// Undefine GetObject macro if defined (to avoid conflicts with RapidJSON)
#ifdef GetObject
#undef GetObject
#endif

namespace Framework {

    // Singleton accessors for registries to ensure initialization order safety
    std::map<std::string, StateCreator>& FSMFactory::GetStateRegistry() {
        static std::map<std::string, StateCreator> registry;
        return registry;
    }

    std::map<std::string, ConditionCreator>& FSMFactory::GetConditionRegistry() {
        static std::map<std::string, ConditionCreator> registry;
        return registry;
    }

    void FSMFactory::RegisterState(const std::string& name, StateCreator creator) {
        GetStateRegistry()[name] = creator;
    }

    void FSMFactory::RegisterCondition(const std::string& name, ConditionCreator creator) {
        GetConditionRegistry()[name] = creator;
    }

    FSM FSMFactory::CreateFromFile(const std::string& filepath) {
        rapidjson::Document doc;
        if (!FileIOSystem::ReadFile(filepath, doc)) { 
            Logger::Get().Error(DebugType::AISystem) << "Failed to load FSM: " << filepath << std::endl;
            return FSM();
        }
        return CreateFromJSON(doc);
    }

    FSM FSMFactory::CreateFromJSON(const rapidjson::Value& root) {
        FSM fsm;

        // 1. Load Initial State Name
        if (root.HasMember("initialState")) {
            fsm.SetInitialState(root["initialState"].GetString());
        }

        // Create States
        if (root.HasMember("states")) {
            const auto& states = root["states"];
            for (auto& s : states.GetArray()) {
                std::string name = s["name"].GetString();
                std::string type = s.HasMember("type") ? s["type"].GetString() : name;

                auto& registry = GetStateRegistry();
                if (registry.find(type) != registry.end()) {
                    auto stateObj = registry[type](); // creation of stateobject

                    // Inject Global Dependencies
                    stateObj->SetNavGrid(navGrid_);

                    // Load Parameters
                    if (s.HasMember("parameters")) {
                        for (auto& p : s["parameters"].GetObject()) {  
                            if (p.value.IsFloat())
                                stateObj->SetParameter(p.name.GetString(), p.value.GetFloat());
                            else if (p.value.IsString())
                                stateObj->SetParameter(p.name.GetString(), p.value.GetString());
                        }
                    }
                    fsm.AddState(name, stateObj);
                }
                else {
                    Logger::Get().Error(DebugType::AISystem) << "Unknown State Type: " << type << std::endl;
                }

            }
        }

        // Create Transitions
        if (root.HasMember("transitions")) {
            for (auto& t : root["transitions"].GetArray()) {
                std::string from = t["from"].GetString();
                std::string to = t["to"].GetString();

                if (t.HasMember("condition")) {
                    const auto& condData = t["condition"];
                    std::string type = condData["type"].GetString();

                    auto& registry = GetConditionRegistry();
                    if (registry.find(type) != registry.end()) {
                        // Create lambda from factory
                        auto conditionFunc = registry[type](condData);
                        fsm.AddTransition(from, to, conditionFunc);
                    }
                }
            }
        }

        return fsm;
    }
}