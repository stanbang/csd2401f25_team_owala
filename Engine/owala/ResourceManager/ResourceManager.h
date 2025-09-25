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
    class AudioSystem; // Forward declaration
    using Texture = Framework::Texture;
	using Audio = FMOD::Sound;
	// Variant to hold different resource types

    class ResourceManager : public ISystem{

    public:

        void Initialize() override;
        void Update(float dt) { /*Empty by design*/ };
        void Shutdown() override {}

        ResourceManager(Graphics& gfx, AudioSystem& audio) : gfx_(&gfx), audiosystem_(&audio) {};

		// Add a copyable resource (like Texture)
        template<typename ResourceType>
        void add(const std::string& name, ResourceType resource);

		// Add a non-copyable resource (like FMOD::Sound)
        template<typename ResourceType>
        void add(const std::string& name, std::unique_ptr<ResourceType> resourcePtr);

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

        bool loadSound(const std::string& id, const char* path, bool loop, bool stream);

    private:
		template <typename ResourceType>
		std::unordered_map<std::string, std::unique_ptr<ResourceType>>& getMap(); // Get the resource map

        std::unordered_map<std::string, std::unique_ptr<Texture>> textureMap; // Texture map
		std::unordered_map<std::string, std::unique_ptr<FMOD::Sound>> soundMap; // Sound map

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
		AudioSystem* audiosystem_ = nullptr; // FMOD system instance for loading sounds
    };


	// Template definitions must be in the header file
    template<typename ResourceType>
    ResourceType& ResourceManager::get(std::string key) {
        std::unordered_map<std::string, std::unique_ptr<ResourceType>>& map = getMap<ResourceType>();
        auto it = map.find(key);

        if (it == map.end()) {
            throw GameException(DebugType::ResourceManager, "Resource with name '" + key + "' not found.");
        }

		// Access the unique_ptr, dereference it, and return the resource
        return *it->second;
    }

	// Overload for raw resource types (copyable types) like Texture
    template<typename ResourceType>
    void ResourceManager::add(const std::string& name, ResourceType resource) {
        std::unordered_map<std::string, std::unique_ptr<ResourceType>>& map = getMap<ResourceType>();

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

	// Overload for unique_ptr (non-copyable types) like FMOD::Sound
    template<typename ResourceType>
    void ResourceManager::add(const std::string& name, std::unique_ptr<ResourceType> resourcePtr) {
        std::unordered_map<std::string, std::unique_ptr<ResourceType>>& map = getMap<ResourceType>();

        if (map.find(name) != map.end()) {
            Logger logger;
            logger.Info(DebugType::ResourceManager, "Resource with name '" + name + "' already exists. Skipping addition.");
            return;
        }

        map.emplace(name, std::move(resourcePtr));
    }

    // Specialization for Texture
    template <>
    inline std::unordered_map<std::string, std::unique_ptr<Framework::Texture>>&
        ResourceManager::getMap<Framework::Texture>() {
        return textureMap;
    }

    // Specialization for FMOD::Sound
    template <>
    inline std::unordered_map<std::string, std::unique_ptr<FMOD::Sound>>&
        ResourceManager::getMap<FMOD::Sound>() {
        return soundMap;
    }

} // namespace Framework
