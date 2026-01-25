/*****************************************************************//**
* \file   PrefabSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*     
* \brief
*      System for loading and managing entity prefabs from JSON files.
*      Supports creating entities from prefab templates with specialized
*      platform types (basic, trap, and scaling platforms). Provides
*      querying and filtering of prefabs by component type.
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
#include <filesystem>

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
        std::string sourceFile;
        int renderLayer = 0;          
        int customRenderOrder = 0;    

        EntityPrefab() : name(""), layer("Default"), renderLayer(0), customRenderOrder(0), sourceFile("") {}
    };

    struct PlatformPrefab {
        std::string textureID;
        float mass;
    };

    struct TrapPlatformPrefab {
        std::string textureID;
        float mass;
        float fallSpeed;
        float triggerDelay;
    };

    class PrefabSystem {
    private:
        std::unordered_map<std::string, EntityPrefab> prefabs_;
        std::string prefabFolderPath_;

        void ParseComponentData(const rapidjson::Value& compObj, ComponentData& outData);
        void DeserializeComponent(EntityStorage& storage, Entity entity, const ComponentData& compData);
        bool LoadSinglePrefabFile(const std::string& filepath);

        PlatformPrefab ExtractPlatformData(const EntityPrefab& prefab) const;
        TrapPlatformPrefab ExtractTrapPlatformData(const EntityPrefab& prefab) const;

    public:
        // Load all prefabs from a folder (scans for .json files)
        bool LoadPrefabsFromFolder(const std::string& folderPath);

        bool LoadPrefabFile(const std::string& filepath);

        bool ReloadAllPrefabs();

        bool LoadPrefabsFromFile(const std::string& filename);

        Entity CreateEntityFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position);
        bool HasPrefab(const std::string& name) const;
        const EntityPrefab* GetPrefab(const std::string& name) const;

        const std::unordered_map<std::string, EntityPrefab>& GetAllPrefabs() const {
            return prefabs_;
        }

        const std::string& GetPrefabFolderPath() const { return prefabFolderPath_; }

        std::unordered_map<std::string, PlatformPrefab> GetPlatformPrefabs() const;
        std::unordered_map<std::string, TrapPlatformPrefab> GetTrapPlatformPrefabs() const;

        const PlatformPrefab* GetPlatformPrefab(const std::string& name) const;
        const TrapPlatformPrefab* GetTrapPlatformPrefab(const std::string& name) const;

        bool UpdatePrefabComponentData(const std::string& prefabName, const std::string& componentType, const rapidjson::Value& newData);
        EntityPrefab* GetMutablePrefab(const std::string& name);

        // Get list of all prefab files in folder
        std::vector<std::string> GetPrefabFileList() const;
        bool SavePrefabToFile(const std::string& prefabName);
    };

} // namespace Framework