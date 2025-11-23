/*****************************************************************//**
* \file   PrefabSystem.cpp
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*      Implementation of the PrefabSystem. Handles JSON parsing of prefab
*      definitions, component deserialization for all component types,
*      and entity instantiation with proper initialization. Includes
*      helper functions to extract and filter platform-specific prefab
*      data based on component composition.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "PrefabSystem.h"
#include "EntityManager.h"
#include "Components.h"
#include "Logger.h"
#include "rapidjson/istreamwrapper.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>


namespace fs = std::filesystem;

namespace Framework {

    bool PrefabSystem::LoadPrefabsFromFolder(const std::string& folderPath) {
        prefabFolderPath_ = folderPath;

        if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Prefab folder not found: " << folderPath << std::endl;
            return false;
        }

        int loadedCount = 0;
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                if (LoadSinglePrefabFile(entry.path().string())) {
                    loadedCount++;
                }
            }
        }

        Logger::Get().Info(DebugType::FileIOSystem)
            << "Loaded " << loadedCount << " prefabs from folder: " << folderPath << std::endl;
        return loadedCount > 0;
    }

    bool PrefabSystem::LoadSinglePrefabFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::Get().Error(DebugType::FileIOSystem)  << "Failed to open prefab file: " << filepath << std::endl;
            return false;
        }

        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        file.close();

        if (!doc.IsObject()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Invalid JSON format: " << filepath << std::endl;
            return false;
        }

        // Single prefab format: { "name": "...", "components": [...], ... }
        if (!doc.HasMember("name") || !doc["name"].IsString()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Prefab missing 'name' field: " << filepath << std::endl;
            return false;
        }

        EntityPrefab prefab;
        prefab.name = doc["name"].GetString();
        prefab.sourceFile = filepath;

        // Parse components
        if (doc.HasMember("components") && doc["components"].IsArray()) {
            const rapidjson::Value& compsArray = doc["components"];
            for (rapidjson::SizeType j = 0; j < compsArray.Size(); ++j) {
                const rapidjson::Value& compObj = compsArray[j];
                if (!compObj.HasMember("type") || !compObj["type"].IsString()) continue;

                ComponentData compData;
                ParseComponentData(compObj, compData);
                prefab.components.push_back(std::move(compData));
            }
        }

        // Parse tags
        if (doc.HasMember("tags") && doc["tags"].IsArray()) {
            const rapidjson::Value& tagsArray = doc["tags"];
            for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
                if (tagsArray[j].IsString()) {
                    prefab.tags.push_back(tagsArray[j].GetString());
                }
            }
        }

        // Parse properties
        if (doc.HasMember("properties") && doc["properties"].IsObject()) {
            const rapidjson::Value& props = doc["properties"];
            if (props.HasMember("layer") && props["layer"].IsString()) {
                prefab.layer = props["layer"].GetString();
            }
        }

        Logger::Get().Info(DebugType::FileIOSystem)
            << "Loaded prefab: " << prefab.name << " from " << filepath << std::endl;
        prefabs_[prefab.name] = std::move(prefab);
        return true;
    }

    bool PrefabSystem::LoadPrefabFile(const std::string& filepath) {
        return LoadSinglePrefabFile(filepath);
    }

    bool PrefabSystem::ReloadAllPrefabs() {
        if (prefabFolderPath_.empty()) {
            Logger::Get().Error(DebugType::FileIOSystem) << "No prefab folder path set" << std::endl;
            return false;
        }
        prefabs_.clear();
        return LoadPrefabsFromFolder(prefabFolderPath_);
    }

    std::vector<std::string> PrefabSystem::GetPrefabFileList() const {
        std::vector<std::string> files;
        if (prefabFolderPath_.empty() || !fs::exists(prefabFolderPath_)) {
            return files;
        }
        for (const auto& entry : fs::directory_iterator(prefabFolderPath_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

    bool PrefabSystem::LoadPrefabsFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Failed to open prefab file: " << filename << std::endl;
            return false;
        }

        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        file.close();

        if (!doc.IsObject() || !doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Invalid prefab file format: " << filename << std::endl;
            return false;
        }

        const rapidjson::Value& prefabsArray = doc["prefabs"];
        for (rapidjson::SizeType i = 0; i < prefabsArray.Size(); ++i) {
            const rapidjson::Value& prefabObj = prefabsArray[i];
            if (!prefabObj.HasMember("name") || !prefabObj["name"].IsString()) continue;

            EntityPrefab prefab;
            prefab.name = prefabObj["name"].GetString();
            prefab.sourceFile = filename;

            if (prefabObj.HasMember("components") && prefabObj["components"].IsArray()) {
                const rapidjson::Value& compsArray = prefabObj["components"];
                for (rapidjson::SizeType j = 0; j < compsArray.Size(); ++j) {
                    const rapidjson::Value& compObj = compsArray[j];
                    if (!compObj.HasMember("type") || !compObj["type"].IsString()) continue;
                    ComponentData compData;
                    ParseComponentData(compObj, compData);
                    prefab.components.push_back(std::move(compData));
                }
            }

            if (prefabObj.HasMember("tags") && prefabObj["tags"].IsArray()) {
                const rapidjson::Value& tagsArray = prefabObj["tags"];
                for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
                    if (tagsArray[j].IsString()) prefab.tags.push_back(tagsArray[j].GetString());
                }
            }

            if (prefabObj.HasMember("properties") && prefabObj["properties"].IsObject()) {
                const rapidjson::Value& props = prefabObj["properties"];
                if (props.HasMember("layer") && props["layer"].IsString()) {
                    prefab.layer = props["layer"].GetString();
                }
            }

            Logger::Get().Info(DebugType::FileIOSystem) << "Loaded prefab: " << prefab.name << std::endl;
            prefabs_[prefab.name] = std::move(prefab);
        }
        return true;
    }

    void PrefabSystem::ParseComponentData(const rapidjson::Value& compObj, ComponentData& outData) {
        outData.type = compObj["type"].GetString();
        if (compObj.HasMember("data") && compObj["data"].IsObject()) {
            outData.data.CopyFrom(compObj["data"], outData.data.GetAllocator());
        }
    }

    Entity PrefabSystem::CreateEntityFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position) {
        auto it = prefabs_.find(prefabName);
        if (it == prefabs_.end()) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Prefab not found: " << prefabName << std::endl;
            return 0;
        }

        const EntityPrefab& prefab = it->second;
        EntityManager entityMgr(storage);
        Entity entity = entityMgr.CreateEntity();
        entityMgr.GetProperties(entity).name = prefabName;

        if (prefab.layer == "Platform") entityMgr.GetProperties(entity).layer = CollisionLayer::Platform;
        else if (prefab.layer == "Player") entityMgr.GetProperties(entity).layer = CollisionLayer::Player;
        else entityMgr.GetProperties(entity).layer = CollisionLayer::Default;

        for (const std::string& tag : prefab.tags) {
            if (tag == "PlatformTag") entityMgr.AddComponent(entity, PlatformTagType);
        }

        for (const ComponentData& compData : prefab.components) {
            DeserializeComponent(storage, entity, compData);
        }

        if (entityMgr.HasComponent(entity, PositionType)) {
            entityMgr.GetComponent<Position>(entity).Pos = position;
        }

        if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
            TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
            trap.originalPosition = position;
        }
        return entity;
    }

    void PrefabSystem::DeserializeComponent(EntityStorage& storage, Entity entity, const ComponentData& compData) {
        EntityManager entityMgr(storage);
        const rapidjson::Document& data = compData.data;

        if (compData.type == "Position") {
            Position pos;
            if (data.HasMember("x")) pos.Pos.x = data["x"].GetFloat();
            if (data.HasMember("y")) pos.Pos.y = data["y"].GetFloat();
            entityMgr.AddComponent(entity, pos);
        }
        else if (compData.type == "Velocity") {
            Velocity vel;
            if (data.HasMember("x")) vel.Vel.x = data["x"].GetFloat();
            if (data.HasMember("y")) vel.Vel.y = data["y"].GetFloat();
            entityMgr.AddComponent(entity, vel);
        }
        else if (compData.type == "RigidBody") {
            RigidBody rb;
            if (data.HasMember("forceX") && data.HasMember("forceY")) {
                rb.Force.x = data["forceX"].GetFloat();
                rb.Force.y = data["forceY"].GetFloat();
            }
            if (data.HasMember("mass")) rb.SetMass(data["mass"].GetFloat());
            entityMgr.AddComponent(entity, rb);
        }
        else if (compData.type == "AABBCollider") {
            Vector2D halfSize(0.5f, 0.5f);
            if (data.HasMember("halfSizeX")) halfSize.x = data["halfSizeX"].GetFloat();
            if (data.HasMember("halfSizeY")) halfSize.y = data["halfSizeY"].GetFloat();
            entityMgr.AddComponent(entity, AABBCollider{ halfSize });
        }
        else if (compData.type == "TextureComponent") {
            std::string texID = "";
            Vector2D halfSize{};
            if (data.HasMember("textureID")) texID = data["textureID"].GetString();
            if (data.HasMember("halfSizeX")) halfSize.x = data["halfSizeX"].GetFloat();
            if (data.HasMember("halfSizeY")) halfSize.y = data["halfSizeY"].GetFloat();
            entityMgr.AddComponent(entity, TextureComponent{ texID, halfSize });
        }
        else if (compData.type == "RotationFlag") {
            entityMgr.AddComponent(entity, RotationFlag{});
        }
        else if (compData.type == "TrapPlatformComponent") {
            TrapPlatformComponent trap;
            if (data.HasMember("originalPositionX")) trap.originalPosition.x = data["originalPositionX"].GetFloat();
            if (data.HasMember("originalPositionY")) trap.originalPosition.y = data["originalPositionY"].GetFloat();
            if (data.HasMember("isTriggered")) trap.isTriggered = data["isTriggered"].GetBool();
            if (data.HasMember("hasFallen")) trap.hasFallen = data["hasFallen"].GetBool();
            if (data.HasMember("currentDelay")) trap.currentDelay = data["currentDelay"].GetFloat();
            if (data.HasMember("currentFallDistance")) trap.currentFallDistance = data["currentFallDistance"].GetFloat();
            if (data.HasMember("fallSpeed")) trap.fallSpeed = data["fallSpeed"].GetFloat();
            if (data.HasMember("triggerDelay")) trap.triggerDelay = data["triggerDelay"].GetFloat();
            entityMgr.AddComponent(entity, trap);
        }
        else {
            std::cout << "Unknown component type: " << compData.type << std::endl;
        }
    }

    bool PrefabSystem::HasPrefab(const std::string& name) const { return prefabs_.find(name) != prefabs_.end(); }
    const EntityPrefab* PrefabSystem::GetPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        return (it != prefabs_.end()) ? &it->second : nullptr;
    }
    EntityPrefab* PrefabSystem::GetMutablePrefab(const std::string& name) {
        auto it = prefabs_.find(name);
        return (it != prefabs_.end()) ? &it->second : nullptr;
    }

    PlatformPrefab PrefabSystem::ExtractPlatformData(const EntityPrefab& prefab) const {
        PlatformPrefab result;
        for (const auto& compData : prefab.components) {
            if (compData.type == "TextureComponent" && compData.data.HasMember("textureID"))
                result.textureID = compData.data["textureID"].GetString();
            if (compData.type == "RigidBody" && compData.data.HasMember("mass"))
                result.mass = compData.data["mass"].GetFloat();
        }
        return result;
    }

    TrapPlatformPrefab PrefabSystem::ExtractTrapPlatformData(const EntityPrefab& prefab) const {
        TrapPlatformPrefab result;
        for (const auto& compData : prefab.components) {
            if (compData.type == "TextureComponent" && compData.data.HasMember("textureID"))
                result.textureID = compData.data["textureID"].GetString();
            if (compData.type == "RigidBody" && compData.data.HasMember("mass"))
                result.mass = compData.data["mass"].GetFloat();
            if (compData.type == "TrapPlatformComponent") {
                if (compData.data.HasMember("fallSpeed")) result.fallSpeed = compData.data["fallSpeed"].GetFloat();
                if (compData.data.HasMember("triggerDelay")) result.triggerDelay = compData.data["triggerDelay"].GetFloat();
            }
        }
        return result;
    }

    std::unordered_map<std::string, PlatformPrefab> PrefabSystem::GetPlatformPrefabs() const {
        std::unordered_map<std::string, PlatformPrefab> platforms;
        for (const auto& pair : prefabs_) {
            const EntityPrefab& prefab = pair.second;
            bool hasTrap = false;
            for (const auto& compData : prefab.components) {
                if (compData.type == "TrapPlatformComponent") hasTrap = true;
             
            }
            if (!hasTrap) platforms[pair.first] = ExtractPlatformData(prefab);
        }
        return platforms;
    }

    std::unordered_map<std::string, TrapPlatformPrefab> PrefabSystem::GetTrapPlatformPrefabs() const {
        std::unordered_map<std::string, TrapPlatformPrefab> trapPlatforms;
        for (const auto& pair : prefabs_) {
            for (const auto& compData : pair.second.components) {
                if (compData.type == "TrapPlatformComponent") {
                    trapPlatforms[pair.first] = ExtractTrapPlatformData(pair.second);
                    break;
                }
            }
        }
        return trapPlatforms;
    }


    const PlatformPrefab* PrefabSystem::GetPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;
        bool hasTrap = false;
        for (const auto& compData : it->second.components) {
            if (compData.type == "TrapPlatformComponent") hasTrap = true;
        }
        if (hasTrap ) return nullptr;
        PlatformPrefab* result = new PlatformPrefab();
        *result = ExtractPlatformData(it->second);
        return result;
    }

    const TrapPlatformPrefab* PrefabSystem::GetTrapPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;
        for (const auto& compData : it->second.components) {
            if (compData.type == "TrapPlatformComponent") {
                TrapPlatformPrefab* result = new TrapPlatformPrefab();
                *result = ExtractTrapPlatformData(it->second);
                return result;
            }
        }
        return nullptr;
    }


    bool PrefabSystem::UpdatePrefabComponentData(const std::string& prefabName,
        const std::string& componentType, const rapidjson::Value& newData) {
        auto it = prefabs_.find(prefabName);
        if (it == prefabs_.end()) return false;
        for (auto& compData : it->second.components) {
            if (compData.type == componentType) {
                compData.data.CopyFrom(newData, compData.data.GetAllocator());
                return true;
            }
        }
        return false;
    }

    bool PrefabSystem::SavePrefabToFile(const std::string& prefabName) {
        auto it = prefabs_.find(prefabName);
        if (it == prefabs_.end()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Prefab not found: " << prefabName << std::endl;
            return false;
        }

        const EntityPrefab& prefab = it->second;

        std::string filePath = prefab.sourceFile;
        if (filePath.empty()) {
            filePath = prefabFolderPath_ + "/" + prefabName + ".json";
        }

        // Create JSON document
        rapidjson::Document doc;
        doc.SetObject();
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        // Add name
        rapidjson::Value nameVal;
        nameVal.SetString(prefab.name.c_str(), alloc);
        doc.AddMember("name", nameVal, alloc);

        // Add components
        rapidjson::Value componentsArray(rapidjson::kArrayType);
        for (const auto& compData : prefab.components) {
            rapidjson::Value compObj(rapidjson::kObjectType);

            rapidjson::Value typeVal;
            typeVal.SetString(compData.type.c_str(), alloc);
            compObj.AddMember("type", typeVal, alloc);

            rapidjson::Value dataVal;
            dataVal.CopyFrom(compData.data, alloc);
            compObj.AddMember("data", dataVal, alloc);

            componentsArray.PushBack(compObj, alloc);
        }
        doc.AddMember("components", componentsArray, alloc);

        // Add tags
        rapidjson::Value tagsArray(rapidjson::kArrayType);
        for (const auto& tag : prefab.tags) {
            rapidjson::Value tagVal;
            tagVal.SetString(tag.c_str(), alloc);
            tagsArray.PushBack(tagVal, alloc);
        }
        doc.AddMember("tags", tagsArray, alloc);

        // Add properties
        rapidjson::Value propsObj(rapidjson::kObjectType);
        rapidjson::Value layerVal;
        layerVal.SetString(prefab.layer.c_str(), alloc);
        propsObj.AddMember("layer", layerVal, alloc);
        doc.AddMember("properties", propsObj, alloc);

        // Write to file
        std::ofstream fileOut(filePath);
        if (!fileOut.is_open()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Failed to open file for writing: " << filePath << std::endl;
            return false;
        }

        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);

        fileOut << buffer.GetString();
        fileOut.close();

        Logger::Get().Info(DebugType::FileIOSystem)
            << "Saved prefab to: " << filePath << std::endl;

        return true;
    }

} // namespace Framework