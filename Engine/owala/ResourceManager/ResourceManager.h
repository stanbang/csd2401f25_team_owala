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
#include <type_traits>

namespace Framework {
    class AudioSystem; // Forward declaration
    using Texture = Framework::Texture;
	using Audio = FMOD::Sound;
	// Variant to hold different resource types

    // Custom deleter for FMOD objects — calls release() instead of delete
    struct FMODDeleter {
        void operator()(FMOD::Sound* s) const noexcept {
            if (s) {
                s->release(); // Release without logging
            }
        }
    };

    class ResourceManager : public ISystem{
    public:

        void Initialize() override;
        void Update(float dt) { /*Empty by design*/ };
        void Shutdown();

        ResourceManager(Graphics& gfx, AudioSystem& audio) : gfx_(&gfx), audiosystem_(&audio) {};

        // Add a copyable resource (like Texture)
        template<typename ResourceType>
        void add(const std::string& name, ResourceType resource);

        // Add a non-copyable resource (like FMOD::Sound) via unique_ptr
        template<typename ResourceType, typename Deleter = std::default_delete<ResourceType>>
        void add(const std::string& name, std::unique_ptr<ResourceType, Deleter> resourcePtr);

		// Get a resource by key
        template<typename ResourceType>
        ResourceType& get(std::string key);

        // Clear resources of a specific type
        template<typename ResourceType>
        void clear() {
			if constexpr (std::is_same<ResourceType, Texture>::value) {
				textureMap.clear();
			}
			else if constexpr (std::is_same<ResourceType, Audio>::value) {
				soundMap.clear();
			}
			else {
				static_assert(sizeof(ResourceType) == 0, "ResourceManager::clear only supports Texture or Audio at the moment.");
			}
        }

        // Clear all resources
        void clearAll();

        // Load a texture from file and store it
        bool loadTexture(const std::string& name, const std::string& filepath);
        bool loadSound(const std::string& id, const char* path, bool loop, bool stream);

        std::unordered_map<std::string, std::unique_ptr<Texture>>& getTextureMap(); // Get the resource map
		std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>>& getSoundMap(); // Get the sound map

    private:

        std::unordered_map<std::string, std::unique_ptr<Texture>> textureMap; // Texture map
		std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>> soundMap; // Sound map with custom deleter

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures
		AudioSystem* audiosystem_ = nullptr; // FMOD system instance for loading sounds
    };


	// Template definitions must be in the header file
    template<typename ResourceType>
    ResourceType& ResourceManager::get(std::string key) {
        if constexpr (std::is_same<ResourceType, Texture>::value) {
            // Lookup in texture map
            return *getTextureMap().at(key);
        }
        else if constexpr (std::is_same<ResourceType, Audio>::value) {
            // Lookup in sound map
            return *getSoundMap().at(key);
        }
        else {
            static_assert(sizeof(ResourceType) == 0, "ResourceManager::get only supports Texture or Audio at the moment.");
        }
    }

    // Copyable resources
    template<typename ResourceType>
    void ResourceManager::add(const std::string& name, ResourceType resource) {
        if constexpr (std::is_same<ResourceType, Texture>::value) {
            auto& map = getTextureMap();
            if (map.find(name) != map.end()) return;
            map.emplace(name, std::make_unique<Texture>(resource));
        }
        else {
            static_assert(sizeof(ResourceType) == 0,
                "Use unique_ptr version of add() for non-copyable resources like Audio.");
        }
    }

    // Non-copyable resources via unique_ptr
    template<typename ResourceType, typename Deleter>
    void ResourceManager::add(const std::string& name, std::unique_ptr<ResourceType, Deleter> resourcePtr) {
        if constexpr (std::is_same<ResourceType, Audio>::value) {
            auto& map = getSoundMap();
            if (map.find(name) != map.end()) return;
            map.emplace(name, std::move(resourcePtr));
        }
        else {
            static_assert(sizeof(ResourceType) == 0,
                "add() unique_ptr version only supports Audio at the moment.");
        }
    }

    // Overload for unique_ptr (non-copyable types like Audio)
    template<typename ResourceType, typename Deleter = std::default_delete<ResourceType>>
    void add(const std::string& name, std::unique_ptr<ResourceType, Deleter> resourcePtr);

} // namespace Framework
