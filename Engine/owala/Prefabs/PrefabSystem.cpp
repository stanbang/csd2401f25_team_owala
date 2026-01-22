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
#include "EntitySerializer.h"
#include "Components.h"
#include "Logger.h"
#include "SerializationKeys.h"
#include "rapidjson/istreamwrapper.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include "FileUtility.h"

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
        for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
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
        rapidjson::Document doc;
        if (!FileIOSystem::ReadFile(filepath, doc)) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Failed to open prefab file: " << filepath << std::endl;
            return false;
        }

        if (!doc.IsObject()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Invalid JSON format: " << filepath << std::endl;
            return false;
        }

        if (!doc.HasMember("name") || !doc["name"].IsString()) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Prefab missing 'name' field: " << filepath << std::endl;
            return false;
        }

        EntityPrefab prefab;
        prefab.name = doc["name"].GetString();
        prefab.sourceFile = filepath;

        if (doc.HasMember(Serialization::COMPONENTS) && doc[Serialization::COMPONENTS].IsObject()) {
            const rapidjson::Value& compsObj = doc[Serialization::COMPONENTS];
            for (auto it = compsObj.MemberBegin(); it != compsObj.MemberEnd(); ++it) {
                ComponentData compData;
                compData.type = it->name.GetString();
                compData.data.CopyFrom(it->value, compData.data.GetAllocator());
                prefab.components.push_back(std::move(compData));
            }
        }

        if (doc.HasMember(Serialization::TAGS) && doc[Serialization::TAGS].IsArray()) {
            const rapidjson::Value& tagsArray = doc[Serialization::TAGS];
            for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
                if (tagsArray[j].IsString()) {
                    prefab.tags.push_back(tagsArray[j].GetString());
                }
            }
        }

        if (doc.HasMember(Serialization::ENTITY_LAYER) && doc[Serialization::ENTITY_LAYER].IsInt()) {
            int layerInt = doc[Serialization::ENTITY_LAYER].GetInt();
            CollisionLayer layer = static_cast<CollisionLayer>(layerInt);
            if (layer == CollisionLayer::Platform) prefab.layer = "Platform";
            else if (layer == CollisionLayer::Player) prefab.layer = "Player";
            else prefab.layer = "Default";
        }

        if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER) && doc[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
            prefab.renderLayer = doc[Serialization::ENTITY_RENDER_LAYER].GetInt();
        }

        if (doc.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER) && doc[Serialization::ENTITY_CUSTOM_RENDER_ORDER].IsInt()) {
            prefab.customRenderOrder = doc[Serialization::ENTITY_CUSTOM_RENDER_ORDER].GetInt();
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
        for (const auto& entry : fs::recursive_directory_iterator(prefabFolderPath_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

    bool PrefabSystem::LoadPrefabsFromFile(const std::string& filename) {
        rapidjson::Document doc;
        if (!FileIOSystem::ReadFile(filename, doc)) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Failed to open prefab file: " << filename << std::endl;
            return false;
        }

        if (!doc.IsObject() || !doc.HasMember("prefabs") || !doc["prefabs"].IsArray()) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Invalid prefab file format: " << filename << std::endl;
            return false;
        }

        const rapidjson::Value& prefabsArray = doc["prefabs"];
        for (rapidjson::SizeType i = 0; i < prefabsArray.Size(); ++i) {
            const rapidjson::Value& prefabObj = prefabsArray[i];
            if (!prefabObj.HasMember("name") || !prefabObj["name"].IsString()) continue;

            EntityPrefab prefab;
            prefab.name = prefabObj["name"].GetString();
            prefab.sourceFile = filename;

            if (prefabObj.HasMember(Serialization::COMPONENTS) && prefabObj[Serialization::COMPONENTS].IsObject()) {
                const rapidjson::Value& compsObj = prefabObj[Serialization::COMPONENTS];
                for (auto it = compsObj.MemberBegin(); it != compsObj.MemberEnd(); ++it) {
                    ComponentData compData;
                    compData.type = it->name.GetString();
                    compData.data.CopyFrom(it->value, compData.data.GetAllocator());
                    prefab.components.push_back(std::move(compData));
                }
            }

            if (prefabObj.HasMember(Serialization::TAGS) && prefabObj[Serialization::TAGS].IsArray()) {
                const rapidjson::Value& tagsArray = prefabObj[Serialization::TAGS];
                for (rapidjson::SizeType j = 0; j < tagsArray.Size(); ++j) {
                    if (tagsArray[j].IsString()) prefab.tags.push_back(tagsArray[j].GetString());
                }
            }

            if (prefabObj.HasMember(Serialization::ENTITY_LAYER) && prefabObj[Serialization::ENTITY_LAYER].IsInt()) {
                int layerInt = prefabObj[Serialization::ENTITY_LAYER].GetInt();
                CollisionLayer layer = static_cast<CollisionLayer>(layerInt);
                if (layer == CollisionLayer::Platform) prefab.layer = "Platform";
                else if (layer == CollisionLayer::Player) prefab.layer = "Player";
                else prefab.layer = "Default";
            }

            if (prefabObj.HasMember(Serialization::ENTITY_RENDER_LAYER) && prefabObj[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
                prefab.renderLayer = prefabObj[Serialization::ENTITY_RENDER_LAYER].GetInt();
            }
            else {
                if (prefab.layer == "Platform") prefab.renderLayer = 50;
                else if (prefab.layer == "Player") prefab.renderLayer = 100;
                else prefab.renderLayer = 0;
            }

            if (prefabObj.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER) && prefabObj[Serialization::ENTITY_CUSTOM_RENDER_ORDER].IsInt()) {
                prefab.customRenderOrder = prefabObj[Serialization::ENTITY_CUSTOM_RENDER_ORDER].GetInt();
            }
            else {
                prefab.customRenderOrder = prefab.renderLayer;
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

        // Set entity properties
        EntityProperties& props = entityMgr.GetProperties(entity);
        props.name = prefabName;

        if (prefab.layer == "Platform") props.layer = CollisionLayer::Platform;
        else if (prefab.layer == "Player") props.layer = CollisionLayer::Player;
        else props.layer = CollisionLayer::Default;

        props.renderLayer = static_cast<RenderLayer>(prefab.renderLayer);
        props.customRenderOrder = prefab.customRenderOrder;

        // Deserialize tags
        for (const std::string& tag : prefab.tags) {
            if (tag == Serialization::PLATFORM_TAG) {
                entityMgr.AddComponent(entity, PlatformTag{});
            }
            else if (tag == Serialization::PLAYER_TAG) {
                entityMgr.AddComponent(entity, PlayerTag{});
            }
            else if (tag == Serialization::BACKGROUND_TAG) {
                entityMgr.AddComponent(entity, BackgroundTag{});
            }
            else if (tag == Serialization::ENEMY_TAG) {
                entityMgr.AddComponent(entity, EnemyTag{});
            }
            else if (tag == Serialization::PERSISTENT_TAG) {
                entityMgr.AddComponent(entity, PersistentTag{});
            }
            else if (tag == Serialization::ACTIVE_TAG) {
                entityMgr.AddComponent(entity, ActiveTag{});
            }
            else if (tag == Serialization::GUI_TAG) {
                entityMgr.AddComponent(entity, GUITag{});
            }
        }

        for (const ComponentData& compData : prefab.components) {
            DeserializeComponent(storage, entity, compData);
        }

        if (entityMgr.HasComponent(entity, TransformType)) {
            entityMgr.GetComponent<Transform>(entity).position = position;
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

        if (compData.type == Serialization::TRANSFORM_COMPONENT) {
            Transform transform;
            if (EntitySerializer::DeserializeComponent(data, transform)) {
                entityMgr.AddComponent(entity, transform);
            }
        }
        else if (compData.type == Serialization::SPAWN_PT_COMPONENT) {
            SpawnPoint spawnPt;
            if (EntitySerializer::DeserializeComponent(data, spawnPt)) {
                entityMgr.AddComponent(entity, spawnPt);
            }
        }
        else if (compData.type == Serialization::VELOCITY_COMPONENT) {
            Velocity vel;
            if (EntitySerializer::DeserializeComponent(data, vel)) {
                entityMgr.AddComponent(entity, vel);
            }
        }
        else if (compData.type == Serialization::ACCELERATION_COMPONENT) {
            Acceleration acc;
            if (EntitySerializer::DeserializeComponent(data, acc)) {
                entityMgr.AddComponent(entity, acc);
            }
        }
        else if (compData.type == Serialization::RIGIDBODY_COMPONENT) {
            RigidBody rb;
            if (EntitySerializer::DeserializeComponent(data, rb)) {
                entityMgr.AddComponent(entity, rb);
            }
        }
        else if (compData.type == Serialization::AABB_COLLIDER_COMPONENT) {
            AABBCollider collider;
            if (EntitySerializer::DeserializeComponent(data, collider)) {
                entityMgr.AddComponent(entity, collider);
            }
        }
        else if (compData.type == Serialization::TEXTURE_COMPONENT) {
            TextureComponent texComp;
            if (EntitySerializer::DeserializeComponent(data, texComp)) {
                entityMgr.AddComponent(entity, texComp);
            }
        }
        else if (compData.type == Serialization::ROTATIONFLAG_COMPONENT) {
            RotationFlag rFlag;
            if (EntitySerializer::DeserializeComponent(data, rFlag)) {
                entityMgr.AddComponent(entity, rFlag);
            }
        }
        else if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
            TrapPlatformComponent trap;
            if (EntitySerializer::DeserializeComponent(data, trap)) {
                entityMgr.AddComponent(entity, trap);
            }
        }
        else if (compData.type == Serialization::AUDIO_COMPONENT) {
            AudioComponent audioComp;
            if (EntitySerializer::DeserializeComponent(data, audioComp)) {
                entityMgr.AddComponent(entity, audioComp);
            }
        }
        else if (compData.type == Serialization::ANIMATION_COMPONENT) {
            AnimationComponent animComp;
            if (EntitySerializer::DeserializeComponent(data, animComp)) {
                entityMgr.AddComponent(entity, animComp);
            }
        }
        else if (compData.type == Serialization::TEXT_COMPONENT) {
            TextComponent textComp;
            if (EntitySerializer::DeserializeComponent(data, textComp)) {
                entityMgr.AddComponent(entity, textComp);
            }
        }
        else if (compData.type == Serialization::PLAYER_STATE_COMPONENT) {
            PlayerState state;
            if (EntitySerializer::DeserializeComponent(data, state)) {
                entityMgr.AddComponent(entity, state);
            }
        }
        else if (compData.type == Serialization::CAMERA_COMPONENT) {
            Camera camera;
            if (EntitySerializer::DeserializeComponent(data, camera)) {
                entityMgr.AddComponent(entity, camera);
            }
        }
        else if (compData.type == Serialization::NATIVE_SCRIPTS_COMPONENT) {
            NativeScriptsComponent scripts;
            if (EntitySerializer::DeserializeComponent(data, scripts)) {
                entityMgr.AddComponent(entity, std::move(scripts));
            }
        }
        else if (compData.type == Serialization::BIRD_AI_COMPONENT) {
            BirdAIComponent birdAI;
            if (EntitySerializer::DeserializeComponent(data, birdAI)) {
                entityMgr.AddComponent(entity, birdAI);
            }
        }
        else if (compData.type == Serialization::SKELETON_AI_COMPONENT) {
            SkeletonAIComponent skelAI;
            if (EntitySerializer::DeserializeComponent(data, skelAI)) {
                entityMgr.AddComponent(entity, skelAI);
            }
        }
        else if (compData.type == Serialization::BUTTON_COMPONENT) {
            Button button;
            if (EntitySerializer::DeserializeComponent(data, button)) {
                entityMgr.AddComponent(entity, button);
            }
        }
        else if (compData.type == Serialization::SCENE_OWNED_COMPONENT) {
            SceneOwned sceneOwned;
            if (EntitySerializer::DeserializeComponent(data, sceneOwned)) {
                entityMgr.AddComponent(entity, sceneOwned);
            }
        }
        else if (compData.type == Serialization::PARTICLE_EMITTER_COMPONENT) {
            ParticleEmitterComponent particleEmitter;
            if (EntitySerializer::DeserializeComponent(data, particleEmitter)) {
                entityMgr.AddComponent(entity, particleEmitter);
            }
        }
        else if (compData.type == Serialization::TRIANGLE_COMPONENT) {
            Triangle triangle;
            if (EntitySerializer::DeserializeComponent(data, triangle)) {
                entityMgr.AddComponent(entity, triangle);
            }
        }
        
        else {
            Logger::Get().Error(DebugType::FileIOSystem) << "Unknown component type: " << compData.type << std::endl;
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

    PlatformPrefab PrefabSystem::ExtractPlatformData(const EntityPrefab& prefab) const {
        PlatformPrefab result;
        for (const auto& compData : prefab.components) {
            if (compData.type == Serialization::TEXTURE_COMPONENT) {
                TextureComponent texComp;
                if (EntitySerializer::DeserializeComponent(compData.data, texComp)) {
                    result.textureID = texComp.textureKey;
                }
            }
            if (compData.type == Serialization::RIGIDBODY_COMPONENT) {
                RigidBody rb;
                if (EntitySerializer::DeserializeComponent(compData.data, rb)) {
                    result.mass = rb.Mass;
                }
            }
        }
        return result;
    }

    TrapPlatformPrefab PrefabSystem::ExtractTrapPlatformData(const EntityPrefab& prefab) const {
        TrapPlatformPrefab result;
        for (const auto& compData : prefab.components) {
            if (compData.type == Serialization::TEXTURE_COMPONENT) {
                TextureComponent texComp;
                if (EntitySerializer::DeserializeComponent(compData.data, texComp)) {
                    result.textureID = texComp.textureKey;
                }
            }
            if (compData.type == Serialization::RIGIDBODY_COMPONENT) {
                RigidBody rb;
                if (EntitySerializer::DeserializeComponent(compData.data, rb)) {
                    result.mass = rb.Mass;
                }
            }
            if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
                TrapPlatformComponent trap;
                if (EntitySerializer::DeserializeComponent(compData.data, trap)) {
                    result.fallSpeed = trap.fallSpeed;
                    result.triggerDelay = trap.triggerDelay;
                }
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
                if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
                    hasTrap = true;
                    break;
                }
            }
            if (!hasTrap) platforms[pair.first] = ExtractPlatformData(prefab);
        }
        return platforms;
    }

    std::unordered_map<std::string, TrapPlatformPrefab> PrefabSystem::GetTrapPlatformPrefabs() const {
        std::unordered_map<std::string, TrapPlatformPrefab> trapPlatforms;
        for (const auto& pair : prefabs_) {
            for (const auto& compData : pair.second.components) {
                if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
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
            if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
                hasTrap = true;
                break;
            }
        }
        if (hasTrap) return nullptr;

        PlatformPrefab* result = new PlatformPrefab();
        *result = ExtractPlatformData(it->second);
        return result;
    }

    const TrapPlatformPrefab* PrefabSystem::GetTrapPlatformPrefab(const std::string& name) const {
        auto it = prefabs_.find(name);
        if (it == prefabs_.end()) return nullptr;

        for (const auto& compData : it->second.components) {
            if (compData.type == Serialization::TRAP_PLATFORM_COMPONENT) {
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
            Logger::Get().Error(DebugType::FileIOSystem) << "Prefab not found: " << prefabName << std::endl;
            return false;
        }

        const EntityPrefab& prefab = it->second;
        std::string filePath = prefab.sourceFile;
        if (filePath.empty()) {
            filePath = prefabFolderPath_ + "/" + prefabName + ".json";
        }

        rapidjson::Document doc;
        if (!FileIOSystem::ReadFile(filePath, doc) || !doc.IsObject()) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Failed to open prefab file for reading: " << filePath << std::endl;
            return false;
        }

        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        if (!doc.HasMember("name") || std::string(doc["name"].GetString()) != prefabName) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Prefab name mismatch in file: " << filePath << std::endl;
            return false;
        }

        if (doc.HasMember(Serialization::COMPONENTS) && doc[Serialization::COMPONENTS].IsObject()) {
            rapidjson::Value& compsObj = doc[Serialization::COMPONENTS];

            compsObj.RemoveAllMembers();

            for (const auto& updatedComp : prefab.components) {
                rapidjson::Value compDataCopy;
                compDataCopy.CopyFrom(updatedComp.data, alloc);

                rapidjson::Value compName;
                compName.SetString(updatedComp.type.c_str(), alloc);

                compsObj.AddMember(compName, compDataCopy, alloc);
            }
        }

        rapidjson::Value tagsArray(rapidjson::kArrayType);
        for (const auto& tag : prefab.tags) {
            rapidjson::Value tagVal;
            tagVal.SetString(tag.c_str(), alloc);
            tagsArray.PushBack(tagVal, alloc);
        }
        doc[Serialization::TAGS] = tagsArray;
        //rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        CollisionLayer layer = CollisionLayer::Default;
        if (prefab.layer == "Platform") layer = CollisionLayer::Platform;
        else if (prefab.layer == "Player") layer = CollisionLayer::Player;

        if (doc.HasMember(Serialization::ENTITY_LAYER)) {
            doc[Serialization::ENTITY_LAYER] = static_cast<int>(layer);
        }
        else {
            rapidjson::Value layerKey(Serialization::ENTITY_LAYER, alloc);
            doc.AddMember(layerKey, static_cast<int>(layer), alloc);
        }

        if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER)) {
            doc[Serialization::ENTITY_RENDER_LAYER] = prefab.renderLayer;
        }
        else {
            rapidjson::Value renderLayerKey(Serialization::ENTITY_RENDER_LAYER, alloc);
            doc.AddMember(renderLayerKey, prefab.renderLayer, alloc);
        }

        if (doc.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER)) {
            doc[Serialization::ENTITY_CUSTOM_RENDER_ORDER] = prefab.customRenderOrder;
        }
        else {
            rapidjson::Value customOrderKey(Serialization::ENTITY_CUSTOM_RENDER_ORDER, alloc);
            doc.AddMember(customOrderKey, prefab.customRenderOrder, alloc);
        }

        FileIOSystem::WriteFileJson(filePath, doc);
        Logger::Get().Info(DebugType::FileIOSystem)
            << "Saved prefab to: " << filePath << std::endl;

        if (std::filesystem::exists(FileIOKeys::ROOT_DATA_FOLDER)) {
            std::string rootFilePath = std::string(FileIOKeys::ROOT_FOLDER_PREFIX) + filePath;
            FileIOSystem::WriteFileJson(rootFilePath, doc);
            Logger::Get().Info(DebugType::FileIOSystem)  << "Saved prefab to: " << rootFilePath << std::endl;
        }

        Logger::Get().Info(DebugType::FileIOSystem) << "Prefab saved successfully: " << prefabName << std::endl;

        return true;
    }
} // namespace Framework