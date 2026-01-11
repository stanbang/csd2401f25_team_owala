/*****************************************************************//**
 * \file   LayerManager.h
 *
 * \brief  Dynamic layer management system for configurable render
 *         and collision layers
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 *********************************************************************/
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "rapidjson/document.h"

namespace Framework {

    /**
     * \struct RenderLayerData
     * \brief Represents a single render layer with customizable properties
     */
    struct RenderLayerData {
        std::string name;           // Display name (e.g., "Background", "Player")
        int renderOrder;            // Sorting order (lower renders first)
        bool isBuiltIn;            // Whether this is a default layer
        bool isVisible;            // Toggle visibility in editor

        RenderLayerData(const std::string& layerName = "New Layer",
            int order = 0,
            bool builtIn = false,
            bool visible = true)
            : name(layerName), renderOrder(order), isBuiltIn(builtIn), isVisible(visible) {}
    };

    /**
     * \struct CollisionLayerData
     * \brief Represents a single collision layer with customizable properties
     */
    struct CollisionLayerData {
        std::string name;           // Display name
        uint8_t id;                // Unique ID (0-31 for bit masking)
        bool isBuiltIn;            // Whether this is a default layer

        CollisionLayerData(const std::string& layerName = "Default",
            uint8_t layerId = 0,
            bool builtIn = false)
            : name(layerName), id(layerId), isBuiltIn(builtIn) {}
    };

    /**
     * \class LayerManager
     * \brief Manages dynamic render and collision layers with persistence
     */
    class LayerManager {
    public:
        LayerManager();
        ~LayerManager() = default;

        // === Render Layer Management ===

        /**
         * \brief Add a new render layer
         * \param name Layer name
         * \param renderOrder Sorting priority
         * \return Index of created layer, or -1 on failure
         */
        int AddRenderLayer(const std::string& name, int renderOrder);

        /**
         * \brief Remove a render layer by index
         * \param index Layer index to remove
         * \return True if removed successfully
         */
        bool RemoveRenderLayer(size_t index);

        /**
         * \brief Update an existing render layer
         * \param index Layer index
         * \param newName New name
         * \param newOrder New render order
         * \return True if updated successfully
         */
        bool UpdateRenderLayer(size_t index, const std::string& newName, int newOrder);

        /**
         * \brief Get all render layers
         */
        const std::vector<RenderLayerData>& GetRenderLayers() const { return renderLayers_; }

        /**
         * \brief Find render layer index by name
         * \return Index if found, -1 otherwise
         */
        int FindRenderLayerByName(const std::string& name) const;

        /**
         * \brief Get render order by layer name
         * \return Render order, or 0 if not found
         */
        int GetRenderOrderByName(const std::string& name) const;

        /**
        * \brief Get render layer name by render order value
        * \param renderOrder The order value to search for
        * \return Layer name if found, empty string otherwise
        */
        std::string GetRenderLayerNameByOrder(int renderOrder) const;

        // === Collision Layer Management ===

        /**
         * \brief Add a new collision layer
         * \param name Layer name
         * \return ID of created layer, or -1 on failure
         */
        int AddCollisionLayer(const std::string& name);

        /**
         * \brief Remove a collision layer by ID
         * \param id Layer ID to remove
         * \return True if removed successfully
         */
        bool RemoveCollisionLayer(uint8_t id);

        /**
         * \brief Rename a collision layer
         * \param id Layer ID
         * \param newName New name
         * \return True if updated successfully
         */
        bool RenameCollisionLayer(uint8_t id, const std::string& newName);

        /**
         * \brief Get all collision layers
         */
        const std::vector<CollisionLayerData>& GetCollisionLayers() const { return collisionLayers_; }

        /**
         * \brief Find collision layer ID by name
         * \return ID if found, -1 otherwise
         */
        int FindCollisionLayerByName(const std::string& name) const;

        /**
         * \brief Get collision layer name by ID
         */
        std::string GetCollisionLayerName(uint8_t id) const;



        // === Persistence ===

        /**
         * \brief Save layer configuration to JSON file
         * \param filepath Path to save location
         * \return True if saved successfully
         */
        bool SaveToFile(const std::string& filepath = "Data/LayerConfig.json");

        /**
         * \brief Load layer configuration from JSON file
         * \param filepath Path to load from
         * \return True if loaded successfully
         */
        bool LoadFromFile(const std::string& filepath = "Data/LayerConfig.json");

        // === Serialization Helpers ===

        void SerializeRenderLayers(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc) const;
        void DeserializeRenderLayers(const rapidjson::Value& inArray);

        void SerializeCollisionLayers(rapidjson::Value& outArray, rapidjson::Document::AllocatorType& alloc) const;
        void DeserializeCollisionLayers(const rapidjson::Value& inArray);

        // === Validation ===

        /**
         * \brief Check if a render layer name already exists
         */
        bool RenderLayerNameExists(const std::string& name) const;

        /**
         * \brief Check if a collision layer name already exists
         */
        bool CollisionLayerNameExists(const std::string& name) const;

        /**
         * \brief Get next available collision layer ID
         * \return Next available ID, or -1 if all slots used
         */
        int GetNextAvailableCollisionID() const;

    private:
        void InitializeDefaultRenderLayers();
        void InitializeDefaultCollisionLayers();

        std::vector<RenderLayerData> renderLayers_;
        std::vector<CollisionLayerData> collisionLayers_;

        // Track used collision IDs for efficient lookup
        std::unordered_map<uint8_t, size_t> collisionIDToIndex_;
    };

} // namespace Framework