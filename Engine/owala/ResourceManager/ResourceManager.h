#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <variant>
#include <iostream>
#include "Graphics.h" // Assuming Framework::Texture is defined here

namespace Framework {

    //using Texture = Framework::Texture;
    //using Value = std::variant<Texture>;

    class ResourceManager : public ISystem{
        //using Value = std::variant<Texture>;
        using Value = std::variant<std::shared_ptr<Texture>>;

    public:

        void Initialize() override;
        void Update(float dt) {/*Empty by design*/ };
        void Shutdown() override {}

        ResourceManager(Graphics& gfx) : gfx_(&gfx) {};

        // Add a resource
        template<typename ResourceType>
        void add(const std::string& name, ResourceType resource) {
            auto& map = getMap();

        //void add(const std::string& name, std::shared_ptr<ResourceType> resource) {
        //    auto& map = getMap<ResourceType>();
            if (map.find(name) != map.end()) {
                std::cout << "Resource with name '" << name << "' already exists. Skipping addition." << std::endl;
                return;
            }
            map[name] = resource;
        }

        // Get a resource, returns weak_ptr to maintain resource ownership by ResourceManager.
        // Convert weak ptr to shared ptr to modify the resource, else weak ptr is READ ONLY!!!

        //template<typename ResourceType>
        //ResourceType& get(std::string key) {
        std::shared_ptr<Texture> get(std::string key) {
            std::unordered_map<std::string, Value>& map = getMap();
            auto it = map.find(key);

            if (it == map.end()) {
                return nullptr; // not found
            }
        //std::weak_ptr<ResourceType> get(const char* name) {
        //    auto& map = getMap<ResourceType>();
        //    auto it = map.find(name);
            return std::get<std::shared_ptr<Texture>>(it->second);
        }

        // Clear resources of a specific type
        template<typename ResourceType>
        void clear() {
            getMap.clear();
            //getMap<ResourceType>().clear();
        }

        // Clear all resources
        void clearAll();

        // Load a texture from file and store it
        bool loadTexture(const std::string& name, const std::string& filepath);

    private:
        // Template declaration
        std::unordered_map<std::string, Value>& getMap();
        //template<typename ResourceType>
        //std::unordered_map<std::string, std::shared_ptr<ResourceType>>& getMap();

        // Resource maps
        std::unordered_map<std::string, Value> resourceMap;
        //std::unordered_map<std::string, std::shared_ptr<Texture>> textureMap;

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
    };

} // namespace Framework
