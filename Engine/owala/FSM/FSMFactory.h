#pragma once
/*****************************************************************//**
* \file   FSMFactory.h
*
* \brief  Factory for creating FSMs from JSON configuration.
*********************************************************************/
#pragma once
#include "FSM.h"
#include "Pathfinding.h"
#include <rapidjson/document.h>
#include <string>

namespace Framework {

    class FSMFactory {
    private:
        NavigationGrid* navGrid_;

    public:
        FSMFactory(NavigationGrid* navGrid) : navGrid_(navGrid) {}

        // Create an FSM from a JSON file
        FSM CreateFromFile(const std::string& filepath);

        // Create an FSM from JSON data
        FSM CreateFromJSON(const rapidjson::Value& data);

    private:
        std::shared_ptr<State> CreateState(const std::string& stateName, const rapidjson::Value& stateData);

        std::function<bool(Entity, EntityStorage&)> CreateCondition(
            const std::string& conditionType,
            const rapidjson::Value& conditionData);
    };

} // namespace Framework