/*****************************************************************//**
* \file   PrefabSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Entity.h"
#include "EntityStorage.h"
#include "rapidjson/document.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Framework {

    // Stores the raw JSON data for a component
    struct ComponentData {
        std::string type;
        rapidjson::Document data;

        ComponentData() : type(""), data() {}
        ComponentData(const std::string& t) : type(t), data() {}
    };

    // Stores complete prefab definition
    struct EntityPrefab {
        std::string name;
        std::vector<ComponentData> components;
        std::vector<std::string> tags;
        std::string layer;

        EntityPrefab() : name(""), layer("Default") {}
    };

    class PrefabSystem {
    private:
        std::unordered_map<std::string, EntityPrefab> prefabs_;

        // Helper to parse component data from JSON
        void ParseComponentData(const rapidjson::Value& compObj, ComponentData& outData);

        // Helper to deserialize specific component types
        void DeserializeComponent(EntityStorage& storage, Entity entity,
            const ComponentData& compData);

    public:
        // Load all prefabs from a JSON file
        bool LoadPrefabsFromFile(const std::string& filename);

        // Create an entity from any prefab
        Entity CreateEntityFromPrefab(EntityStorage& storage,const std::string& prefabName,  const Vector2D& position);

        // Check if prefab exists
        bool HasPrefab(const std::string& name) const;

        // Get prefab data
        const EntityPrefab* GetPrefab(const std::string& name) const;

        // Get all loaded prefabs
        const std::unordered_map<std::string, EntityPrefab>& GetAllPrefabs() const {
            return prefabs_;
        }
    };

} // namespace Framework