/*********************************************************************//**
* \file   FSMFactory.h
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Declares the FSMFactory class and registration macros used to map
* string identifiers to C++ state and condition classes for data-driven
* creation.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "FSM.h"
#include <rapidjson/document.h>
#include <functional>
#include <map>
#include <string>

namespace Framework {

    class NavigationGrid;

    // Signature for functions that create States/Conditions
    using StateCreator = std::function<std::shared_ptr<State>()>;
    using ConditionCreator = std::function<std::function<bool(Entity, EntityStorage&)>(const rapidjson::Value&)>;

    class FSMFactory {
    private:
        NavigationGrid* navGrid_;

        // Static Registries (Shared across all factory instances)
        static std::map<std::string, StateCreator>& GetStateRegistry();
        static std::map<std::string, ConditionCreator>& GetConditionRegistry();

    public:
        FSMFactory(NavigationGrid* grid) : navGrid_(grid) {}

        FSM CreateFromFile(const std::string& filepath);
        FSM CreateFromJSON(const rapidjson::Value& root);

        // Registration Helpers (Used by Macros)
        static void RegisterState(const std::string& name, StateCreator creator);
        static void RegisterCondition(const std::string& name, ConditionCreator creator);
    };

    // --- MACROS FOR AUTO-REGISTRATION ---

    // Use in .cpp files of your States: REGISTER_STATE("Chase", BirdChaseState)
#define REGISTER_STATE(NAME, CLASS) \
        static struct Register##CLASS { \
            Register##CLASS() { \
                FSMFactory::RegisterState(NAME, []() { return std::make_shared<CLASS>(); }); \
            } \
        } register##CLASS##Instance;

    // Use to register condition helper functions
#define REGISTER_CONDITION(NAME, FUNC) \
        static struct RegisterCond##NAME { \
            RegisterCond##NAME() { \
                FSMFactory::RegisterCondition(#NAME, FUNC); \
            } \
        } registerCond##NAME##Instance;

}