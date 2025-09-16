#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include "Graphics.h" // Assuming Framework::Texture is defined here

namespace Framework {

    using Texture = Framework::Texture;

    class ResourceManager {
    public:

        ResourceManager(Graphics& gfx) : gfx_(&gfx) {};

        // Add a resource
        template<typename ResourceType>
        void add(const std::string& name, std::shared_ptr<ResourceType> resource) {
            auto& map = getMap<ResourceType>();
            if (map.find(name) != map.end()) {
                std::cout << "Resource with name '" << name << "' already exists. Skipping addition." << std::endl;
                return;
            }
            map[name] = resource;
        }

        // Get a resource
        template<typename ResourceType>
        std::shared_ptr<ResourceType> get(const char* name) {
            auto& map = getMap<ResourceType>();
            auto it = map.find(name);
            return (it != map.end()) ? it->second : nullptr;
        }

        // Clear resources of a specific type
        template<typename ResourceType>
        void clear() {
            getMap<ResourceType>().clear();
        }

        // Clear all resources
        void clearAll();

        // Load a texture from file and store it
        bool loadTexture(const std::string& name, const std::string& filepath);

    private:
        // Template declaration
        template<typename ResourceType>
        std::unordered_map<std::string, std::shared_ptr<ResourceType>>& getMap();

        // Resource maps
        std::unordered_map<std::string, std::shared_ptr<Texture>> textureMap;

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
    };

} // namespace Framework
