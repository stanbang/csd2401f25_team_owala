/*****************************************************************//**
 * \file   ResourceManager.h
 * \brief  
 * 
 * \author Ng Jing Wen
 * \date   September 2025
 *********************************************************************/
#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <variant>
#include <iostream>
#include "Graphics.h" // Assuming Framework::Texture is defined here
#include "Logger.h"
#include <fmod.hpp>

namespace Framework {

    using Texture = Framework::Texture;
	// Variant to hold different resource types
    using Value = std::variant<
        std::unique_ptr<Texture>, 
        std::unique_ptr<FMOD::Sound>
    >;

    class ResourceManager : public ISystem{

    public:

        void Initialize() override;
        void Update(float dt) { /*Empty by design*/ };
        void Shutdown() override {}

        ResourceManager(Graphics& gfx) : gfx_(&gfx) {};

        // Add a resource
        template<typename ResourceType>
        void add(const std::string& name, ResourceType resource);

		// Get a resource by key
        template<typename ResourceType>
        ResourceType& get(std::string key);

        // Clear resources of a specific type
        template<typename ResourceType>
        void clear() {
            getMap.clear();
        }

        // Clear all resources
        void clearAll();

        // Load a texture from file and store it
        bool loadTexture(const std::string& name, const std::string& filepath);

    private:
		std::unordered_map<std::string, Value>& getMap(); // Get the resource map
        std::unordered_map<std::string, Value> resourceMap; // Resource maps
		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
    };


	// Template definitions must be in the header file
    template<typename ResourceType>
    ResourceType& ResourceManager::get(std::string key) {
        std::unordered_map<std::string, Value>& map = getMap();
        auto it = map.find(key);

        if (it == map.end()) {
            throw GameException(DebugType::ResourceManager, "Resource with name '" + key + "' not found.");
        }

        // Access the unique_ptr in the variant, then dereference it
        return *std::get<std::unique_ptr<ResourceType>>(it->second);
    }

    template<typename ResourceType>
    void ResourceManager::add(const std::string& name, ResourceType resource) {
        std::unordered_map<std::string, Value>& map = getMap();

        // Convert raw resource to unique_ptr
        std::unique_ptr<ResourceType> resourcePtr = std::make_unique<ResourceType>(resource);

        if (map.find(name) != map.end()) {
            Logger logger;
            logger.Info(DebugType::ResourceManager, "Resource with name '" + name + "already exists. Skipping addition.");
            return;
        }

        // Use emplace to insert the unique_ptr directly into the variant
        map.emplace(name, std::move(resourcePtr));
    }

} // namespace Framework
