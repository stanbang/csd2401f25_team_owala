#include "PrefabSystem.h"
#include "EntityManager.h"
#include "Components.h"
#include "Logger.h"
#include "rapidjson/istreamwrapper.h"
#include <fstream>
#include <iostream>

namespace Framework {

    bool PrefabSystem::LoadPrefabsFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Failed to open prefab file: " << filename << std::endl;
            return false;
        }

        rapidjson::IStreamWrapper isw(file);
        rapidjson::Document doc;
        doc.ParseStream(isw);
        file.close();

        if (!doc.IsObject() || !doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
            Logger::Get().Error(DebugType::FileIOSystem)  << "Invalid prefab file format: " << filename << std::endl;
            return false;
        }

        const rapidjson::Value& prefabsArray = doc["prefabs"];

        for (rapidjson::SizeType i = 0; i < prefabsArray.Size(); ++i) {
            const rapidjson::Value& prefabObj = prefabsArray[i];

            if (!prefabObj.HasMember("name") || !prefabObj["name"].IsString()) {
               std::cout << "Skipping prefab without valid name" << std::endl;
                continue;
            }

            std::string prefabName = prefabObj["name"].GetString();
            EntityPrefab prefab;
            prefab.name = prefabName;

            // Parse components
            if (prefabObj.HasMember("components") && prefabObj["components"].IsArray()) {
                const rapidjson::Value& compsArray = prefabObj["components"];

                for (rapidjson::SizeType j = 0; j < compsArray.Size(); ++j) {
                    const rapidjson::Value& compObj = compsArray[j];

                    if (!compObj.HasMember("type") || !compObj["type"].IsString()) {
                        continue;
                    }

                    ComponentData compData;
                    ParseComponentData(compObj, compData);
                    prefab.components.push_back(std::move(compData));
                }
            }

            // Parse tags
            if (prefabObj.HasMember("tags") && prefabObj["tags"].IsArray()) {
                const rapidjson::Value& tagsArray = prefabObj["tags"];
                for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
                    if (tagsArray[j].IsString()) {
                        prefab.tags.push_back(tagsArray[j].GetString());
                    }
                }
            }

            // Parse properties
            if (prefabObj.HasMember("properties") && prefabObj["properties"].IsObject()) {
                const rapidjson::Value& props = prefabObj["properties"];
                if (props.HasMember("layer") && props["layer"].IsString()) {
                    prefab.layer = props["layer"].GetString();
                }
            }
            Logger::Get().Info(DebugType::FileIOSystem) << "Loaded prefab: " << prefabName << std::endl;
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
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Prefab not found: " << prefabName << std::endl;
            return 0;
        }

        const EntityPrefab& prefab = it->second;
        EntityManager entityMgr(storage);

        // Create base entity
        Entity entity = entityMgr.CreateEntity();

        // Set entity name
        entityMgr.GetProperties(entity).name = prefabName;

        // Set collision layer
        if (prefab.layer == "Platform") {
            entityMgr.GetProperties(entity).layer = CollisionLayer::Platform;
        }
        else if (prefab.layer == "Player") {
            entityMgr.GetProperties(entity).layer = CollisionLayer::Player;
        }
        else {
            entityMgr.GetProperties(entity).layer = CollisionLayer::Default;
        }

        for (const std::string& tag : prefab.tags) {
            if (tag == "PlatformTag") {
                entityMgr.AddComponent(entity, PlatformTagType);
            }
        }

        // Add all components from prefab definition
        for (const ComponentData& compData : prefab.components) {
            DeserializeComponent(storage, entity, compData);
        }

        if (entityMgr.HasComponent(entity, PositionType)) {
            entityMgr.GetComponent<Position>(entity).Pos = position;
        }        
        if (entityMgr.HasComponent(entity, SpawnPointType)) {
            entityMgr.GetComponent<SpawnPoint>(entity).Pos = position;
        }

        if (entityMgr.HasComponent(entity, ScalingPlatformComponentType) &&
            entityMgr.HasComponent(entity, AABBColliderType)) {
            ScalingPlatformComponent& scale = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
            AABBCollider& col = entityMgr.GetComponent<AABBCollider>(entity);
            scale.baseHalfSize = col.HalfSize;
        }

        if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
            TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(entity);
            trap.originalPosition = position;
        }
        return entity;
    }

    void PrefabSystem::DeserializeComponent(EntityStorage& storage,Entity entity,const ComponentData& compData) { 
        EntityManager entityMgr(storage);
        const rapidjson::Document& data = compData.data;

        // Deserialize based on component type
        if (compData.type == "Position") {
            Position pos;
            if (data.HasMember("x")) pos.Pos.x = data["x"].GetFloat();
            if (data.HasMember("y")) pos.Pos.y = data["y"].GetFloat();
            entityMgr.AddComponent(entity, pos);
        }
        else if (compData.type == "SpawnPoint") {
            SpawnPoint spawnPt;
            if (data.HasMember("x")) spawnPt.Pos.x = data["x"].GetFloat();
            if (data.HasMember("y")) spawnPt.Pos.y = data["y"].GetFloat();
            entityMgr.AddComponent(entity, spawnPt);
        }
        else if (compData.type == "Velocity") {
            Velocity vel;
            if (data.HasMember("x")) vel.Vel.x = data["x"].GetFloat();
            if (data.HasMember("y")) vel.Vel.y = data["y"].GetFloat();
            entityMgr.AddComponent(entity, vel);
        }
        else if (compData.type == "RigidBody") {
            RigidBody rb;

            // Set the initial force
            if (data.HasMember("forceX") && data.HasMember("forceY")) {
                rb.Force.x = data["forceX"].GetFloat();
                rb.Force.y = data["forceY"].GetFloat();
            }

            if (data.HasMember("mass")) {
                rb.SetMass(data["mass"].GetFloat());
            }

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
            if (data.HasMember("textureID")) texID = data["textureID"].GetString();
            entityMgr.AddComponent(entity, TextureComponent{ texID });
        }
        else if (compData.type == "RotationFlag") {
            entityMgr.AddComponent(entity, RotationFlag{});
        }
        else if (compData.type == "TrapPlatformComponent") {
            TrapPlatformComponent trap;
            if (data.HasMember("originalPositionX"))
                trap.originalPosition.x = data["originalPositionX"].GetFloat();
            if (data.HasMember("originalPositionY"))
                trap.originalPosition.y = data["originalPositionY"].GetFloat();
            if (data.HasMember("isTriggered"))
                trap.isTriggered = data["isTriggered"].GetBool();
            if (data.HasMember("hasFallen"))
                trap.hasFallen = data["hasFallen"].GetBool();
            if (data.HasMember("currentDelay"))
                trap.currentDelay = data["currentDelay"].GetFloat();
            if (data.HasMember("currentFallDistance"))
                trap.currentFallDistance = data["currentFallDistance"].GetFloat();
            if (data.HasMember("fallSpeed"))
                trap.fallSpeed = data["fallSpeed"].GetFloat();
            if (data.HasMember("triggerDelay"))
                trap.triggerDelay = data["triggerDelay"].GetFloat();
            entityMgr.AddComponent(entity, trap);
        }
        else if (compData.type == "ScalingPlatformComponent") {
            ScalingPlatformComponent scaling;
         /*   if (data.HasMember("baseHalfSizeX"))
                scaling.baseHalfSize.x = data["baseHalfSizeX"].GetFloat();
            if (data.HasMember("baseHalfSizeY"))
                scaling.baseHalfSize.y = data["baseHalfSizeY"].GetFloat();*/
            if (data.HasMember("currentScaleX"))
                scaling.currentScaleX = data["currentScaleX"].GetFloat();
            if (data.HasMember("currentScaleY"))
                scaling.currentScaleY = data["currentScaleY"].GetFloat();
            if (data.HasMember("scaleSpeed"))
                scaling.scaleSpeed = data["scaleSpeed"].GetFloat();
            if (data.HasMember("minScale"))
                scaling.minScale = data["minScale"].GetFloat();
            if (data.HasMember("maxScale"))
                scaling.maxScale = data["maxScale"].GetFloat();
            if (data.HasMember("time"))
                scaling.time = data["time"].GetFloat();
            entityMgr.AddComponent(entity, scaling);
        }
        else {
            std::cout<< "Unknown component type: " << compData.type << std::endl;
        }
    }

    bool PrefabSystem::HasPrefab(const std::string& name) const {
        return prefabs_.find(name) != prefabs_.end();
    }

    const EntityPrefab* PrefabSystem::GetPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        return (it != prefabs_.end()) ? &it->second : nullptr;
    }

    EntityPrefab* PrefabSystem::GetMutablePrefab(const std::string& name) {
        auto it = prefabs_.find(name);
        return (it != prefabs_.end()) ? &it->second : nullptr;
    }

    // ========== HELPER FUNCTIONS TO GET SPECIFIC TYPE PLATFORMS DATA ==========

    PlatformPrefab PrefabSystem::ExtractPlatformData(const EntityPrefab& prefab) const {
        PlatformPrefab result;

        for (const auto& compData : prefab.components) {
            if (compData.type == "TextureComponent" && compData.data.HasMember("textureID")) {
                result.textureID = compData.data["textureID"].GetString();
            }
            if (compData.type == "RigidBody" && compData.data.HasMember("mass")) {
                result.mass = compData.data["mass"].GetFloat();
            }
        }

        return result;
    }

    TrapPlatformPrefab PrefabSystem::ExtractTrapPlatformData(const EntityPrefab& prefab) const {
        TrapPlatformPrefab result;

        for (const auto& compData : prefab.components) {
            if (compData.type == "TextureComponent" && compData.data.HasMember("textureID")) {
                result.textureID = compData.data["textureID"].GetString();
            }
            if (compData.type == "RigidBody" && compData.data.HasMember("mass")) {
                result.mass = compData.data["mass"].GetFloat();
            }
            if (compData.type == "TrapPlatformComponent") {
                if (compData.data.HasMember("fallSpeed"))
                    result.fallSpeed = compData.data["fallSpeed"].GetFloat();
                if (compData.data.HasMember("triggerDelay"))
                    result.triggerDelay = compData.data["triggerDelay"].GetFloat();
            }
        }

        return result;
    }

    ScalingPlatformPrefab PrefabSystem::ExtractScalingPlatformData(const EntityPrefab& prefab) const {
        ScalingPlatformPrefab result;

        for (const auto& compData : prefab.components) {
            if (compData.type == "TextureComponent" && compData.data.HasMember("textureID")) {
                result.textureID = compData.data["textureID"].GetString();
            }
            if (compData.type == "RigidBody" && compData.data.HasMember("mass")) {
                result.mass = compData.data["mass"].GetFloat();
            }
            if (compData.type == "ScalingPlatformComponent") {
                if (compData.data.HasMember("scaleSpeed"))
                    result.scaleSpeed = compData.data["scaleSpeed"].GetFloat();
                if (compData.data.HasMember("minScale"))
                    result.minScale = compData.data["minScale"].GetFloat();
                if (compData.data.HasMember("maxScale"))
                    result.maxScale = compData.data["maxScale"].GetFloat();
            }
        }

        return result;
    }

    std::unordered_map<std::string, PlatformPrefab> PrefabSystem::GetPlatformPrefabs() const {
        std::unordered_map<std::string, PlatformPrefab> platforms;

        for (const auto& pair : prefabs_) {
            const EntityPrefab& prefab = pair.second;

            // Check if it's a basic platform
            bool hasTrap = false;
            bool hasScaling = false;

            for (const auto& compData : prefab.components) {
                if (compData.type == "TrapPlatformComponent") hasTrap = true;
                if (compData.type == "ScalingPlatformComponent") hasScaling = true;
            }

            if (!hasTrap && !hasScaling) {
                platforms[pair.first] = ExtractPlatformData(prefab);
            }
        }

        return platforms;
    }

    std::unordered_map<std::string, TrapPlatformPrefab> PrefabSystem::GetTrapPlatformPrefabs() const {
        std::unordered_map<std::string, TrapPlatformPrefab> trapPlatforms;

        for (const auto& pair : prefabs_) {
            const EntityPrefab& prefab = pair.second;

            // Check if it has TrapPlatformComponent
            for (const auto& compData : prefab.components) {
                if (compData.type == "TrapPlatformComponent") {
                    trapPlatforms[pair.first] = ExtractTrapPlatformData(prefab);
                    break;
                }
            }
        }

        return trapPlatforms;
    }

    std::unordered_map<std::string, ScalingPlatformPrefab> PrefabSystem::GetScalingPlatformPrefabs() const {
        std::unordered_map<std::string, ScalingPlatformPrefab> scalingPlatforms;

        for (const auto& pair : prefabs_) {
            const EntityPrefab& prefab = pair.second;

            // Check if it has ScalingPlatformComponent
            for (const auto& compData : prefab.components) {
                if (compData.type == "ScalingPlatformComponent") {
                    scalingPlatforms[pair.first] = ExtractScalingPlatformData(prefab);
                    break;
                }
            }
        }

        return scalingPlatforms;
    }

    const PlatformPrefab* PrefabSystem::GetPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;

        const EntityPrefab& prefab = it->second;

        bool hasTrap = false;
        bool hasScaling = false;

        for (const auto& compData : prefab.components) {
            if (compData.type == "TrapPlatformComponent") hasTrap = true;
            if (compData.type == "ScalingPlatformComponent") hasScaling = true;
        }

        if (hasTrap || hasScaling) return nullptr;

        PlatformPrefab* result = new PlatformPrefab();
        *result = ExtractPlatformData(prefab);
        return result;
    }

    const TrapPlatformPrefab* PrefabSystem::GetTrapPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;

        const EntityPrefab& prefab = it->second;

        for (const auto& compData : prefab.components) {
            if (compData.type == "TrapPlatformComponent") {
                TrapPlatformPrefab* result = new TrapPlatformPrefab();
                *result = ExtractTrapPlatformData(prefab);
                return result;
            }
        }

        return nullptr;
    }

    const ScalingPlatformPrefab* PrefabSystem::GetScalingPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;

        const EntityPrefab& prefab = it->second;

        for (const auto& compData : prefab.components) {
            if (compData.type == "ScalingPlatformComponent") {
                ScalingPlatformPrefab* result = new ScalingPlatformPrefab();
                *result = ExtractScalingPlatformData(prefab);
                return result;
            }
        }

        return nullptr;
    }


    bool PrefabSystem::UpdatePrefabComponentData(const std::string& prefabName,
        const std::string& componentType,
        const rapidjson::Value& newData) {
        auto it = prefabs_.find(prefabName);
        if (it == prefabs_.end()) return false;

        EntityPrefab& prefab = it->second;

        // Find the component and update its data
        for (auto& compData : prefab.components) {
            if (compData.type == componentType) {
                compData.data.CopyFrom(newData, compData.data.GetAllocator());
                return true;
            }
        }

        return false;
    }

} // namespace Framework