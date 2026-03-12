/*****************************************************************//**
 * \file   LayerManager.cpp
 * 
 * \primary_author
 *     Lur Pei Hao Stanley 
 *
 * \brief  Implementation of dynamic layer management system
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 *********************************************************************/
#include "pch.h"
#include "LayerManager.h"
#include "FileUtility.h"
#include "Logger.h"
#include "PhysicsSystem.h"

namespace Framework {

    // Registers all built-in render and collision layers on construction.
    LayerManager::LayerManager() {
        InitializeDefaultRenderLayers();
        InitializeDefaultCollisionLayers();

      
    }

    // Registers the five built-in render layers: Background, Platform, Enemy, Player, UI.
    void LayerManager::InitializeDefaultRenderLayers() {
        // pre built layers for legacy layers
        renderLayers_.push_back(RenderLayerData("Background", -100, true));
        renderLayers_.push_back(RenderLayerData("Platform", 50, true));
        renderLayers_.push_back(RenderLayerData("Enemy", 75, true));
        renderLayers_.push_back(RenderLayerData("Player", 100, true));
        renderLayers_.push_back(RenderLayerData("UI", 200, true));
    }

    // Registers the five built-in collision layers and builds the initial ID-to-index map.
    void LayerManager::InitializeDefaultCollisionLayers() {
        // built-in collision layers
        collisionLayers_.push_back(CollisionLayerData("Default", 0, true));
        collisionLayers_.push_back(CollisionLayerData("Player", 1, true));
        collisionLayers_.push_back(CollisionLayerData("Platform", 2, true));
        collisionLayers_.push_back(CollisionLayerData("Enemy", 3, true));
        collisionLayers_.push_back(CollisionLayerData("Trigger", 4, true));

        // Build index map
        for (size_t i = 0; i < collisionLayers_.size(); ++i) {
            collisionIDToIndex_[collisionLayers_[i].id] = i;
        }
    }

    // === Render Layer Management ===

    // Adds a new render layer. Keeps the list sorted by renderOrder.
    // Returns the new layer's index, or -1 if the name already exists.
    int LayerManager::AddRenderLayer(const std::string& name, int renderOrder) {
        if (RenderLayerNameExists(name)) {
            Logger::Get().Error(DebugType::Editor)
                << "Render layer '" << name << "' already exists!" << std::endl;
            return -1;
        }

        renderLayers_.push_back(RenderLayerData(name, renderOrder, false));

        // Sort by render order
        std::sort(renderLayers_.begin(), renderLayers_.end(),
            [](const RenderLayerData& a, const RenderLayerData& b) {
                return a.renderOrder < b.renderOrder;
            });

        return FindRenderLayerByName(name);
    }

    // Removes a render layer by index. Built-in layers cannot be removed.
    // Returns false if the index is out of range or the layer is built-in.
    bool LayerManager::RemoveRenderLayer(size_t index) {
        if (index >= renderLayers_.size()) return false;

        // Prevent removal of built-in layers
        if (renderLayers_[index].isBuiltIn) {
            Logger::Get().Error(DebugType::Editor)
                << "Cannot remove built-in render layer!" << std::endl;
            return false;
        }

        renderLayers_.erase(renderLayers_.begin() + index);
        return true;
    }

    // Renames a render layer and changes its draw order. Re-sorts the list afterward.
    // Returns false if the index is invalid or the new name is already taken.
    bool LayerManager::UpdateRenderLayer(size_t index, const std::string& newName, int newOrder) {
        if (index >= renderLayers_.size()) return false;

        // Check for same name
        if (newName != renderLayers_[index].name && RenderLayerNameExists(newName)) {
            Logger::Get().Error(DebugType::Editor)
                << "Render layer name '" << newName << "' already exists!" << std::endl;
            return false;
        }

        renderLayers_[index].name = newName;
        renderLayers_[index].renderOrder = newOrder;

        // Re-sort after update
        std::sort(renderLayers_.begin(), renderLayers_.end(),
            [](const RenderLayerData& a, const RenderLayerData& b) {
                return a.renderOrder < b.renderOrder;
            });

        return true;
    }

    // Linear search for a render layer by name. Returns its index, or -1 if not found.
    int LayerManager::FindRenderLayerByName(const std::string& name) const {
        for (size_t i = 0; i < renderLayers_.size(); ++i) {
            if (renderLayers_[i].name == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    // Returns the draw order value for a named render layer, or 0 if it does not exist.
    int LayerManager::GetRenderOrderByName(const std::string& name) const {
        int idx = FindRenderLayerByName(name);
        if (idx >= 0) {
            return renderLayers_[idx].renderOrder;
        }
        return 0; // Default order
    }   

    // Returns the name of the render layer with the given draw order, or empty string if not found.
    std::string LayerManager::GetRenderLayerNameByOrder(int renderOrder) const {
        for (const auto& layer : renderLayers_) {
            if (layer.renderOrder == renderOrder) {
                return layer.name;
            }
        }
        return ""; // Not found
    }

    // Returns true if a render layer with the given name is already registered.
    bool LayerManager::RenderLayerNameExists(const std::string& name) const {
        return FindRenderLayerByName(name) >= 0;
    }

    // === Collision Layer Management ===

    // Adds a new collision layer with the next available ID (0-31).
    // Returns the assigned ID, or -1 if the name is taken or all 32 slots are used.
    int LayerManager::AddCollisionLayer(const std::string& name) {
        if (CollisionLayerNameExists(name)) {
            Logger::Get().Error(DebugType::Editor)
                << "Collision layer '" << name << "' already exists!" << std::endl;
            return -1;
        }

        int nextID = GetNextAvailableCollisionID();
        if (nextID < 0) {
            Logger::Get().Error(DebugType::Editor)
                << "Maximum collision layers reached (32)!" << std::endl;
            return -1;
        }

        uint8_t id = static_cast<uint8_t>(nextID);
        collisionLayers_.push_back(CollisionLayerData(name, id, false));
        collisionIDToIndex_[id] = collisionLayers_.size() - 1;

        return id;
    }

    // Removes a collision layer by ID and rebuilds the index map.
    // Built-in layers cannot be removed. Returns false if the ID is invalid.
    bool LayerManager::RemoveCollisionLayer(uint8_t id) {
        auto it = collisionIDToIndex_.find(id);
        if (it == collisionIDToIndex_.end()) return false;

        size_t index = it->second;

        // Prevent removal of built-in layers
        if (collisionLayers_[index].isBuiltIn) {
            Logger::Get().Error(DebugType::Editor)
                << "Cannot remove built-in collision layer!" << std::endl;
            return false;
        }

        collisionLayers_.erase(collisionLayers_.begin() + index);
        collisionIDToIndex_.erase(it);

        // Rebuild index map
        collisionIDToIndex_.clear();
        for (size_t i = 0; i < collisionLayers_.size(); ++i) {
            collisionIDToIndex_[collisionLayers_[i].id] = i;
        }

        return true;
    }

    // Renames a collision layer, rejecting the change if the new name is already taken.
    bool LayerManager::RenameCollisionLayer(uint8_t id, const std::string& newName) {
        auto it = collisionIDToIndex_.find(id);
        if (it == collisionIDToIndex_.end()) return false;

        size_t index = it->second;

        // Check name conflict
        if (newName != collisionLayers_[index].name && CollisionLayerNameExists(newName)) {
            Logger::Get().Error(DebugType::Editor)
                << "Collision layer name '" << newName << "' already exists!" << std::endl;
            return false;
        }

        collisionLayers_[index].name = newName;
        return true;
    }

    // Returns the ID of a collision layer by name, or -1 if not found.
    int LayerManager::FindCollisionLayerByName(const std::string& name) const {
        for (const auto& layer : collisionLayers_) {
            if (layer.name == name) {
                return layer.id;
            }
        }
        return -1;
    }

    // Returns the name associated with a collision layer ID, or "Unknown" if not found.
    std::string LayerManager::GetCollisionLayerName(uint8_t id) const {
        auto it = collisionIDToIndex_.find(id);
        if (it != collisionIDToIndex_.end()) {
            return collisionLayers_[it->second].name;
        }
        return "Unknown";
    }

    // Returns true if a collision layer with the given name is already registered.
    bool LayerManager::CollisionLayerNameExists(const std::string& name) const {
        return FindCollisionLayerByName(name) >= 0;
    }

    // Finds the first unused collision ID in the range 0-31. Returns -1 if all slots are taken.
    int LayerManager::GetNextAvailableCollisionID() const {
        for (int i = 0; i < 32; ++i) {
            if (collisionIDToIndex_.find(static_cast<uint8_t>(i)) == collisionIDToIndex_.end()) {
                return i;
            }
        }
        return -1;
    }

    // Resizes and resets the internal collision matrix so all layer pairs collide by default.
    void LayerManager::RebuildCollisionMatrix() {
        size_t layerCount = collisionLayers_.size();
        collisionMatrix_.resize(layerCount);

        for (size_t i = 0; i < layerCount; ++i) {
            collisionMatrix_[i].resize(layerCount, true); // Default: all collide
        }
    }

    // Sets the collision flag between two layers symmetrically (both directions match).
    void LayerManager::SetLayerCollision(uint8_t id1, uint8_t id2, bool canCollide) {
        auto it1 = collisionIDToIndex_.find(id1);
        auto it2 = collisionIDToIndex_.find(id2);

        if (it1 != collisionIDToIndex_.end() && it2 != collisionIDToIndex_.end()) {
            size_t idx1 = it1->second;
            size_t idx2 = it2->second;

            // Set symmetrically
            collisionMatrix_[idx1][idx2] = canCollide;
            collisionMatrix_[idx2][idx1] = canCollide;
        }
    }

    // Returns whether two collision layers are set to interact. Returns false if either ID is invalid.
    bool LayerManager::GetLayerCollision(uint8_t id1, uint8_t id2) const {
        auto it1 = collisionIDToIndex_.find(id1);
        auto it2 = collisionIDToIndex_.find(id2);

        if (it1 != collisionIDToIndex_.end() && it2 != collisionIDToIndex_.end()) {
            size_t idx1 = it1->second;
            size_t idx2 = it2->second;

            if (idx1 < collisionMatrix_.size() && idx2 < collisionMatrix_[idx1].size()) {
                return collisionMatrix_[idx1][idx2];
            }
        }
        return false;
    }


    // === Persistence ===

    // Serializes all render layers, collision layers, and collision matrix to a JSON file.
    // Returns false on IO error or exception.
    bool LayerManager::SaveToFile(const std::string& filepath, PhysicsSystem* physicsSystem) {
        try {
            rapidjson::Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();

            // Serialize render layers
            rapidjson::Value renderArray(rapidjson::kArrayType);
            SerializeRenderLayers(renderArray, alloc);
            doc.AddMember("RenderLayers", renderArray, alloc);

            // Serialize collision layers
            rapidjson::Value collisionArray(rapidjson::kArrayType);
            SerializeCollisionLayers(collisionArray, alloc);
            doc.AddMember("CollisionLayers", collisionArray, alloc);

            if (physicsSystem) {
                rapidjson::Value matrixArray(rapidjson::kArrayType);
                SerializeCollisionMatrix(matrixArray, alloc, physicsSystem);
                doc.AddMember("CollisionMatrix", matrixArray, alloc);
            }

            // Write to Data folder
            if (!FileIOSystem::WriteFileJson(filepath, doc)) {
                Logger::Get().Error(DebugType::FileIOSystem)
                    << "Failed to write layer config to: " << filepath << std::endl;
                return false;
            }
            Logger::Get().Info(DebugType::FileIOSystem)
                << "Successfully saved layer config to: " << filepath << std::endl;

            return true;
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::FileIOSystem)
                << "Exception while saving layer config: " << e.what() << std::endl;
            return false;
        }
    }

    // Loads render layers, collision layers, and collision matrix from a JSON file.
    // If the file is missing, the defaults set in the constructor remain active.
    bool LayerManager::LoadFromFile(const std::string& filepath, PhysicsSystem* physicsSystem) {
        try {
            rapidjson::Document doc;
            if (!FileIOSystem::ReadFile(filepath, doc)) {
                Logger::Get().Error(DebugType::Editor)
                    << "No layer config found at " << filepath << ", using defaults" << std::endl;
                return false;
            }

            if (!doc.IsObject()) {
                Logger::Get().Error(DebugType::Editor)
                    << "Layer config file is not a valid JSON object" << std::endl;
                return false;
            }

            if (doc.HasMember("RenderLayers") && doc["RenderLayers"].IsArray()) {
                DeserializeRenderLayers(doc["RenderLayers"]);
            }

            if (doc.HasMember("CollisionLayers") && doc["CollisionLayers"].IsArray()) {
                DeserializeCollisionLayers(doc["CollisionLayers"]);
            }

            if (doc.HasMember("CollisionMatrix") && doc["CollisionMatrix"].IsArray()) {
                DeserializeCollisionMatrix(doc["CollisionMatrix"], physicsSystem);
            }
            else {
                // If no matrix saved, use defaults (all collide)
                RebuildCollisionMatrix();
            }

            Logger::Get().Info(DebugType::Editor)
                << "Loaded layer configuration from " << filepath << std::endl;
            return true;
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::Editor)
                << "Failed to load layers: " << e.what() << std::endl;
            return false;
        }
    }

    // Writes all render layer entries (name, order, flags) into a rapidjson array.
    void LayerManager::SerializeRenderLayers(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc) const {
        for (const auto& layer : renderLayers_) {
            rapidjson::Value obj(rapidjson::kObjectType);

            rapidjson::Value nameVal;
            nameVal.SetString(layer.name.c_str(), alloc);
            obj.AddMember("name", nameVal, alloc);
            obj.AddMember("renderOrder", layer.renderOrder, alloc);
            obj.AddMember("isBuiltIn", layer.isBuiltIn, alloc);
            obj.AddMember("isVisible", layer.isVisible, alloc);

            outArray.PushBack(obj, alloc);
        }
    }

    // Clears and rebuilds the render layer list from a rapidjson array.
    void LayerManager::DeserializeRenderLayers(const rapidjson::Value& inArray) {
        renderLayers_.clear();

        for (rapidjson::SizeType i = 0; i < inArray.Size(); ++i) {
            const auto& obj = inArray[i];

            RenderLayerData layer;
            if (obj.HasMember("name") && obj["name"].IsString()) {
                layer.name = obj["name"].GetString();
            }
            if (obj.HasMember("renderOrder") && obj["renderOrder"].IsInt()) {
                layer.renderOrder = obj["renderOrder"].GetInt();
            }
            if (obj.HasMember("isBuiltIn") && obj["isBuiltIn"].IsBool()) {
                layer.isBuiltIn = obj["isBuiltIn"].GetBool();
            }
            if (obj.HasMember("isVisible") && obj["isVisible"].IsBool()) {
                layer.isVisible = obj["isVisible"].GetBool();
            }

            renderLayers_.push_back(layer);
        }
    }

    // Writes all collision layer entries (name, id, flags) into a rapidjson array.
    void LayerManager::SerializeCollisionLayers(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc) const {
        for (const auto& layer : collisionLayers_) {
            rapidjson::Value obj(rapidjson::kObjectType);

            rapidjson::Value nameVal;
            nameVal.SetString(layer.name.c_str(), alloc);
            obj.AddMember("name", nameVal, alloc);
            obj.AddMember("id", static_cast<int>(layer.id), alloc);
            obj.AddMember("isBuiltIn", layer.isBuiltIn, alloc);

            outArray.PushBack(obj, alloc);
        }
    }

    // Clears and rebuilds the collision layer list and ID map from a rapidjson array.
    void LayerManager::DeserializeCollisionLayers(const rapidjson::Value& inArray) {
        collisionLayers_.clear();
        collisionIDToIndex_.clear();

        for (rapidjson::SizeType i = 0; i < inArray.Size(); ++i) {
            const auto& obj = inArray[i];

            CollisionLayerData layer;
            if (obj.HasMember("name") && obj["name"].IsString()) {
                layer.name = obj["name"].GetString();
            }
            if (obj.HasMember("id") && obj["id"].IsInt()) {
                layer.id = static_cast<uint8_t>(obj["id"].GetInt());
            }
            if (obj.HasMember("isBuiltIn") && obj["isBuiltIn"].IsBool()) {
                layer.isBuiltIn = obj["isBuiltIn"].GetBool();
            }

            collisionIDToIndex_[layer.id] = collisionLayers_.size();
            collisionLayers_.push_back(layer);
        }
    }

    // Writes all unique layer-pair collision flags into a rapidjson array.
    // Queries the PhysicsSystem for the current state of each pair.
    void LayerManager::SerializeCollisionMatrix(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc, 
        PhysicsSystem* physics) const {
        // For each pair of layers, save whether they collide
        for (size_t i = 0; i < collisionLayers_.size(); ++i) {
            for (size_t j = i; j < collisionLayers_.size(); ++j) {

                uint8_t id1 = collisionLayers_[i].id;
                uint8_t id2 = collisionLayers_[j].id;

                
                bool canCollide = physics->GetLayerCollision(
                    static_cast<CollisionLayer>(id1),
                    static_cast<CollisionLayer>(id2)
                );

                rapidjson::Value obj(rapidjson::kObjectType);
                obj.AddMember("layer1", id1, alloc);
                obj.AddMember("layer2", id2, alloc);
                obj.AddMember("canCollide", canCollide, alloc);

                outArray.PushBack(obj, alloc);
               
            }
        }
    }

    // Resets the matrix to all-collide, then applies each saved pair entry to the PhysicsSystem.
    void LayerManager::DeserializeCollisionMatrix(const rapidjson::Value& inArray, PhysicsSystem* physics) {
        // Rebuild matrix to default state first
        RebuildCollisionMatrix();

        // Load saved collision settings
        for (rapidjson::SizeType i = 0; i < inArray.Size(); ++i) {
            const auto& obj = inArray[i];

            if (obj.HasMember("layer1") && obj.HasMember("layer2") && obj.HasMember("canCollide")) {
                uint8_t layer1 = static_cast<uint8_t>(obj["layer1"].GetInt());
                uint8_t layer2 = static_cast<uint8_t>(obj["layer2"].GetInt());
                bool canCollide = obj["canCollide"].GetBool();

                physics->SetLayerCollision(
                    static_cast<CollisionLayer>(layer1),
                    static_cast<CollisionLayer>(layer2),
                    canCollide
                );
               // SetLayerCollision(layer1, layer2, canCollide);
            }
        }
    }

} // namespace Framework