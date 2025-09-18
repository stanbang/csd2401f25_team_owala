#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <variant>
#include <iostream>
#include "Graphics.h" // Assuming Framework::Texture is defined here

namespace Framework {
    using Value = std::variant<Framework::Texture>;

    class ResourceManager : public ISystem{
    public:

        void Initialize() override;
        void Update(float dt) {/*Empty by design*/ };
        void Shutdown() override {}

        ResourceManager(Graphics& gfx) : gfx_(&gfx) {};

        // Add a resource
        template<typename ResourceType>
        void add(const std::string& name, ResourceType resource) {
            auto& map = getMap();
            if (map.find(name) != map.end()) {
                std::cout << "Resource with name '" << name << "' already exists. Skipping addition." << std::endl;
                return;
            }
            map[name] = resource;
        }

        // Get a resource, returns weak_ptr to maintain resource ownership by ResourceManager.
        // Convert weak ptr to shared ptr to modify the resource, else weak ptr is READ ONLY!!!
        template<typename ResourceType>
        ResourceType& get(std::string key) {
            std::unordered_map<std::string, Value>& map = getMap();
            auto it = map.find(key);
            return (it != map.end()) ? it->second : nullptr;
        }

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
        // Template declaration
        std::unordered_map<std::string, Value>& getMap();

        // Resource maps
        std::unordered_map<std::string, Value> resourceMap;

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
    };

} // namespace Framework
