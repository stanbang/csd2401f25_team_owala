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
#include "pch.h"
#include "PrefabSystem.h"
#include "EntityManager.h"
#include "EntitySerializer.h"
#include "Components.h"
#include "Logger.h"
#include "SerializationKeys.h"
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
            prefab.layer = doc[Serialization::ENTITY_LAYER].GetInt();
        }

        if (doc.HasMember(Serialization::ENTITY_COLLISION_LAYER_NAME) && doc[Serialization::ENTITY_COLLISION_LAYER_NAME].IsString()) {
            prefab.layer_collision_name = doc[Serialization::ENTITY_COLLISION_LAYER_NAME].GetString();
        }

        if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER) && doc[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
            prefab.renderLayer = doc[Serialization::ENTITY_RENDER_LAYER].GetInt();
        }

        if (doc.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER) && doc[Serialization::ENTITY_CUSTOM_RENDER_ORDER].IsInt()) {
            prefab.customRenderOrder = doc[Serialization::ENTITY_CUSTOM_RENDER_ORDER].GetInt();
        }

        if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER_NAME) && doc[Serialization::ENTITY_RENDER_LAYER_NAME].IsString()) {
            prefab.layer_render_name = doc[Serialization::ENTITY_RENDER_LAYER_NAME].GetString();
        }

        //Logger::Get().Info(DebugType::FileIOSystem) << "Loaded prefab: " << prefab.name << " from " << filepath << std::endl;
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
                prefab.layer = prefabObj[Serialization::ENTITY_LAYER].GetInt();
            }

            if (prefabObj.HasMember(Serialization::ENTITY_COLLISION_LAYER_NAME) && prefabObj[Serialization::ENTITY_COLLISION_LAYER_NAME].IsString()) {
                prefab.layer_collision_name = prefabObj[Serialization::ENTITY_COLLISION_LAYER_NAME].GetString();
            }

            if (prefabObj.HasMember(Serialization::ENTITY_RENDER_LAYER) && prefabObj[Serialization::ENTITY_RENDER_LAYER].IsInt()) {
                prefab.renderLayer = prefabObj[Serialization::ENTITY_RENDER_LAYER].GetInt();
            }

            if (prefabObj.HasMember(Serialization::ENTITY_CUSTOM_RENDER_ORDER) && prefabObj[Serialization::ENTITY_CUSTOM_RENDER_ORDER].IsInt()) {
                prefab.customRenderOrder = prefabObj[Serialization::ENTITY_CUSTOM_RENDER_ORDER].GetInt();
            }

            if (prefabObj.HasMember(Serialization::ENTITY_RENDER_LAYER_NAME) && prefabObj[Serialization::ENTITY_RENDER_LAYER_NAME].IsString()) {
                prefab.layer_render_name = prefabObj[Serialization::ENTITY_RENDER_LAYER_NAME].GetString();
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

        // Set collision layer
        props.layer = static_cast<CollisionLayer>(prefab.layer);
        props.collisionLayerName = prefab.layer_collision_name;

        props.renderLayer = static_cast<RenderLayer>(prefab.renderLayer);
        props.customRenderOrder = prefab.customRenderOrder;
        props.renderLayerName = prefab.layer_render_name;

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
                //entityMgr.AddComponent(entity, ActiveTag{});
            }
            else if (tag == Serialization::GUI_TAG) {
                entityMgr.AddComponent(entity, GUITag{});
            }
            else if (tag == Serialization::SPAWN_PT_TAG) {
                entityMgr.AddComponent(entity, SpawnPointTag{});
            }
        }

        for (const ComponentData& compData : prefab.components) {
            DeserializeComponent(storage, entity, compData);
        }
        if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
            const std::string& entityName = entityMgr.GetProperties(entity).name;
            const bool isRopePlatform = (entityName.find("Vertical") != std::string::npos ||
                entityName.find("Horizontal") != std::string::npos) &&
                entityName.find("MultiAxis") == std::string::npos;

            if (isRopePlatform) {
                bool hasRopeScript = false;
                if (entityMgr.HasComponent(entity, NativeScriptsComponentType)) {
                    NativeScriptsComponent& scripts = entityMgr.GetComponent<NativeScriptsComponent>(entity);
                    for (auto& script : scripts.scripts) {
                        if (script.serializedTypeName == "RopeScript" ||
                            (script.scriptName && std::string(script.scriptName) == "RopeScript")) {
                            script.executeInEditMode = true;
                            hasRopeScript = true;
                            break;
                        }
                    }
                }

                if (!hasRopeScript) {
                    std::size_t idx = entityMgr.AttachScriptByName(entity, "RopeScript", true);
                    if (idx != static_cast<std::size_t>(-1)) {
                        NativeScriptsComponent& scripts = entityMgr.GetComponent<NativeScriptsComponent>(entity);
                        if (idx < scripts.scripts.size()) {
                            scripts.scripts[idx].executeInEditMode = true;
                        }
                    }
                }
            }
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

    bool PrefabSystem::SaveEntityAsPrefab(EntityStorage& storage, Entity entity, const std::string& prefabName, const std::string& customFolder) {
        using namespace Framework;

        EntityManager entityMgr(storage);

        if (!entityMgr.ValidateEntity(entity)) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Invalid entity provided for prefab creation" << std::endl;
            return false;
        }

        std::string customFolderPath = prefabFolderPath_;
        if (!customFolder.empty()) {
            customFolderPath = prefabFolderPath_ + "/" + customFolder;
        }

        if (!fs::exists(customFolderPath)) {
            try {
                fs::create_directories(customFolderPath);
                Logger::Get().Info(DebugType::FileIOSystem) << "Created custom prefab folder: " << customFolderPath << std::endl;
            }
            catch (const std::exception& e) {
                Logger::Get().Error(DebugType::FileIOSystem) << "Failed to create custom prefab folder: " << e.what() << std::endl;
                return false;
            }
        }

        std::string filepath = customFolderPath + "/" + prefabName + ".json";
        if (fs::exists(filepath)) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Prefab file already exists and will be overwritten: " << filepath << std::endl;
        }

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        doc.AddMember("name", rapidjson::Value(prefabName.c_str(), alloc), alloc);
        EntityProperties& props = entityMgr.GetProperties(entity);
        if (entityMgr.HasComponent(entity, TrapPlatformComponentType)) {
            const std::string& entityName = entityMgr.GetProperties(entity).name;
            const bool isRopePlatform = (entityName.find("Vertical") != std::string::npos ||
                entityName.find("Horizontal") != std::string::npos) &&
                entityName.find("MultiAxis") == std::string::npos;

            if (isRopePlatform) {
                bool hasRopeScript = false;
                if (entityMgr.HasComponent(entity, NativeScriptsComponentType)) {
                    NativeScriptsComponent& scripts = entityMgr.GetComponent<NativeScriptsComponent>(entity);
                    for (auto& script : scripts.scripts) {
                        if (script.serializedTypeName == "RopeScript" ||
                            (script.scriptName && std::string(script.scriptName) == "RopeScript")) {
                            script.executeInEditMode = true;
                            hasRopeScript = true;
                            break;
                        }
                    }
                }

                if (!hasRopeScript) {
                    std::size_t idx = entityMgr.AttachScriptByName(entity, "RopeScript", true);
                    if (idx != static_cast<std::size_t>(-1)) {
                        NativeScriptsComponent& scripts = entityMgr.GetComponent<NativeScriptsComponent>(entity);
                        if (idx < scripts.scripts.size()) {
                            scripts.scripts[idx].executeInEditMode = true;
                        }
                    }
                }
            }
        }

        // Save collision layer ID
        doc.AddMember(rapidjson::Value(Serialization::ENTITY_LAYER, alloc).Move(),
            rapidjson::Value(static_cast<int>(props.layer)).Move(),
            alloc);

        // Save collision layer name
        if (!props.collisionLayerName.empty()) {
            doc.AddMember(rapidjson::Value(Serialization::ENTITY_COLLISION_LAYER_NAME, alloc).Move(),
                rapidjson::Value(props.collisionLayerName.c_str(), alloc),
                alloc);
        }

        // Save render layer ID
        doc.AddMember(rapidjson::Value(Serialization::ENTITY_RENDER_LAYER, alloc).Move(),
            rapidjson::Value(static_cast<int>(props.renderLayer)).Move(),
            alloc);

        // Save custom render order
        doc.AddMember(rapidjson::Value(Serialization::ENTITY_CUSTOM_RENDER_ORDER, alloc).Move(),
            rapidjson::Value(props.customRenderOrder).Move(),
            alloc);

        // Save render layer name
        if (!props.renderLayerName.empty()) {
            doc.AddMember(rapidjson::Value(Serialization::ENTITY_RENDER_LAYER_NAME, alloc).Move(),
                rapidjson::Value(props.renderLayerName.c_str(), alloc),
                alloc);
        }

        rapidjson::Value componentsObj(rapidjson::kObjectType);
        EntitySerializer::SerializeComponents(entityMgr, entity, componentsObj, alloc);
        if (componentsObj.HasMember(Serialization::COMPONENTS)) {
            doc.AddMember(rapidjson::Value(Serialization::COMPONENTS, alloc).Move(),
                componentsObj[Serialization::COMPONENTS],
                alloc);
        }

        rapidjson::Value tagsArray(rapidjson::kArrayType);
        if (entityMgr.HasComponent(entity, PlayerTagType)) {
            tagsArray.PushBack(rapidjson::Value(Serialization::PLAYER_TAG, alloc), alloc);
        }
        if (entityMgr.HasComponent(entity, PlatformTagType)) {
            tagsArray.PushBack(rapidjson::Value(Serialization::PLATFORM_TAG, alloc), alloc);
        }
        if (entityMgr.HasComponent(entity, BackgroundTagType)) {
            tagsArray.PushBack(rapidjson::Value(Serialization::BACKGROUND_TAG, alloc), alloc);
        }
        if (entityMgr.HasComponent(entity, EnemyTagType)) {
            tagsArray.PushBack(rapidjson::Value(Serialization::ENEMY_TAG, alloc), alloc);
        }
        if (entityMgr.HasComponent(entity, GUITagType)) {
            tagsArray.PushBack(rapidjson::Value(Serialization::GUI_TAG, alloc), alloc);
        }

        if (!tagsArray.Empty()) {
            doc.AddMember(rapidjson::Value(Serialization::TAGS, alloc).Move(),
                tagsArray,
                alloc);
        }

        // Write to file
        if (!FileIOSystem::WriteFileJson(filepath, doc)) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Failed to write prefab file: " << filepath << std::endl;
            return false;
        }

        Logger::Get().Info(DebugType::FileIOSystem) << "Successfully saved custom prefab: " << filepath << std::endl;

        if (LoadSinglePrefabFile(filepath)) {
            Logger::Get().Info(DebugType::FileIOSystem) << "Loaded new prefab '" << prefabName << "' into prefab system" << std::endl;
        }

        return true;
    }
    bool PrefabSystem::DeletePrefab(const std::string& prefabName) {
        const EntityPrefab* prefab = GetPrefab(prefabName);
        if (!prefab) {
            Logger::Get().Error(DebugType::FileIOSystem) << "Prefab not found: " << prefabName << std::endl;
            return false;
        }

        if (FileIOSystem::DeleteFileJson(prefab->sourceFile)) {
            Logger::Get().Info(DebugType::FileIOSystem)  << "Deleted prefab file: " << prefab->sourceFile << std::endl;
        }
        else {
            Logger::Get().Error(DebugType::FileIOSystem) << "Failed to delete prefab file: " << prefab->sourceFile << std::endl;
            return false;
        }

        prefabs_.erase(prefabName);
        Logger::Get().Info(DebugType::FileIOSystem) << "Removed prefab from system: " << prefabName << std::endl;
        return true;
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
        /*else if (compData.type == Serialization::BIRD_AI_COMPONENT) {
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
        }*/
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
        if (doc.HasMember(Serialization::TAGS)) {
            doc[Serialization::TAGS] = tagsArray;
        }
        else {
            rapidjson::Value tagsKey(Serialization::TAGS, alloc);
            doc.AddMember(tagsKey, tagsArray, alloc);
        }
        //rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();

        // Save collision layer ID
        if (doc.HasMember(Serialization::ENTITY_LAYER)) {
            doc[Serialization::ENTITY_LAYER] = prefab.layer;
        }
        else {
            rapidjson::Value layerKey(Serialization::ENTITY_LAYER, alloc);
            doc.AddMember(layerKey, prefab.layer, alloc);
        }

        // Save collision layer name
        if (doc.HasMember(Serialization::ENTITY_COLLISION_LAYER_NAME)) {
            doc[Serialization::ENTITY_COLLISION_LAYER_NAME].SetString(prefab.layer_collision_name.c_str(), alloc);
        }
        else {
            rapidjson::Value layerCollisionNameKey(Serialization::ENTITY_COLLISION_LAYER_NAME, alloc);
            rapidjson::Value layerCollisionNameVal(prefab.layer_collision_name.c_str(), alloc);
            doc.AddMember(layerCollisionNameKey, layerCollisionNameVal, alloc);
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

        // Save render layer name
        if (!prefab.layer_render_name.empty()) {
            if (doc.HasMember(Serialization::ENTITY_RENDER_LAYER_NAME)) {
                doc[Serialization::ENTITY_RENDER_LAYER_NAME].SetString(prefab.layer_render_name.c_str(), alloc);
            }
            else {
                rapidjson::Value renderLayerNameKey(Serialization::ENTITY_RENDER_LAYER_NAME, alloc);
                rapidjson::Value renderLayerNameVal(prefab.layer_render_name.c_str(), alloc);
                doc.AddMember(renderLayerNameKey, renderLayerNameVal, alloc);
            }
        }

        if (FileIOSystem::WriteFileJson(filePath, doc)) {
            Logger::Get().Info(DebugType::FileIOSystem) << 
                "Saved prefab <" << prefabName << "> to: " << filePath << std::endl;
        }

        return true;
    }
} // namespace Framework
