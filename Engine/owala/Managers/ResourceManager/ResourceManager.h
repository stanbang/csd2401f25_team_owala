/*****************************************************************//**
* \file   ResourceManager.h
*
* \primary_author
*      Ng Jing Wen (80%)
* \secondary_author
*      Yar Xuan (20%)
*
* \brief    Contains declarations and definitions of a ResourceManager utilising templates
*           that can load and store various resource types like textures, audio and fonts.
*           It maintains ownership of resources using smart pointers with custom deleters.
*           Ensuring that resources are properly released when no longer needed or out of scope.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
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
#include "Font.h"
#include "Animation.h"
#include <filesystem>

namespace Framework {
    class AudioSystem; // Forward declaration
    using Texture = Framework::Texture;
	using Audio = FMOD::Sound;
    namespace fs = std::filesystem;

	// Custom deleter for Texture objects
    struct TextureDeleter {
        void operator()(Texture* texPtr) const noexcept;
    };

    // Custom deleter for FMOD objects — calls release() instead of delete
    struct FMODDeleter {
        void operator()(FMOD::Sound* s) const noexcept;
    };

    class ResourceManager : public ISystem{
    public:

        void Initialize() override;
        void Update(float dt);
        void Shutdown();

		// Constructor requires Graphics and AudioSystem instances for loading resources
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

		// Check if a resource exists
        template<typename ResourceType>
        bool has(const std::string& key) const;

        template<typename ResourceType>
        void unload(const std::string& key) {
            if (std::is_same<ResourceType, Texture>::value) {
                textureMap.erase(key); // Erase from texture map
            }
            else if (std::is_same<ResourceType, Audio>::value) {
                soundMap.erase(key); // Erase from sound map
            }
            else if (std::is_same<ResourceType, Font>::value) {
                fontMap.erase(key); // Erase from font map
            }
            else if (std::is_same<ResourceType, AnimationInfo>::value) {
                animationMap.erase(key); // Erase from animation map
            }
            else {
                Logger::Get().Info(DebugType::ResourceManager) << "Unload only supported for Texture, Audio, Font and AnimationInfo\n";
            }
        }
        // Clear resources of a specific type
        template<typename ResourceType>
        void clear() {
			if constexpr (std::is_same<ResourceType, Texture>::value) {
				textureMap.clear(); // Clear texture map
			}
			else if constexpr (std::is_same<ResourceType, Audio>::value) {
				soundMap.clear(); // Clear sound map
			}
			else if constexpr (std::is_same<ResourceType, Font>::value) {
				fontMap.clear(); // Clear font map
			}
			else if constexpr (std::is_same<ResourceType, AnimationInfo>::value) {
				animationMap.clear(); // Clear animation map
			}
			else {
				static_assert(sizeof(ResourceType) == 0, "ResourceManager::clear only supports Texture, Audio, Fonts or AnimationData at the moment.");
			}
        }

        // Clear all resources
        void clearAll();

        // Scan a directory for assets and load them
        void ScanAssetDirectory(const fs::path& directoryPath);
        // Load a asset by name (assuming name maps to a file path)
        bool loadTexture(const std::string& name);
        bool loadSound(const std::string& name);
        bool loadFont(const std::string& name);
        bool loadAnimationInfo(const std::string& name);

		// Load assets from file and store it in their respective maps
		bool loadTexture(const std::string& name, const std::string& filepath); // Overload with filepath
        bool loadSound(const std::string& id, const char* path, bool loop, bool stream);
        bool loadFont(const std::string& fontKey, const std::string& fontPath, unsigned int size = 48);
		bool loadAnimationInfo(const std::string& name, const std::string& filepath);

		std::unordered_map<std::string, std::unique_ptr<Texture, TextureDeleter>>& getTextureMap(); // Get the texture map
		std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>>& getSoundMap(); // Get the sound map
        std::unordered_map<std::string, std::unique_ptr<Font>>& getFontMap(); // Get the font map
		std::unordered_map<std::string, std::unique_ptr<AnimationInfo>>& getAnimationMap(); // Get the animation map

    private:
        std::unordered_map<std::string, std::unique_ptr<Texture, TextureDeleter>> textureMap; // Texture map
		std::unordered_map<std::string, std::unique_ptr<Audio, FMODDeleter>> soundMap; // Sound map with custom deleter
        std::unordered_map<std::string, std::unique_ptr<Font>> fontMap; // font map
		std::unordered_map<std::string, std::unique_ptr<AnimationInfo>> animationMap; // Animation map
		std::unordered_map<std::string, fs::path> assetPathMap_; // Map of asset names to file paths

		Graphics* gfx_ = nullptr; // Graphics instance for loading textures

        AudioSystem* audiosystem_ = nullptr; // FMOD system instance for loading sounds
    };

    template<typename ResourceType>
    ResourceType& ResourceManager::get(std::string key) {

        // Texture
        if constexpr (std::is_same<ResourceType, Texture>::value) {
            auto& map = getTextureMap();
            auto it = map.find(key);
            if (it == map.end()) {
				// Load automatically if missing
                if (loadTexture(key)) {
                    it = map.find(key); // Try to find it again
                    if (it != map.end()) return *it->second;
                }

                Logger::Get().Error(DebugType::ResourceManager)
                    << "[ResourceManager] Missing Texture: \"" << key << "\"\n";
                throw std::runtime_error("Missing Texture: " + key);
            }
            return *it->second;
        }

        // Audio
        else if constexpr (std::is_same<ResourceType, Audio>::value) {
            auto& map = getSoundMap();
            auto it = map.find(key);
            if (it == map.end()) {
				// Load automatically if missing
                if (loadSound(key)) {
                    it = map.find(key);
                    if (it != map.end()) return *it->second;
                }

                Logger::Get().Error(DebugType::ResourceManager)
                    << "[ResourceManager] Missing Audio: \"" << key << "\"\n";
                throw std::runtime_error("Missing Audio: " + key);
            }
            return *it->second;
        }

        // Font
        else if constexpr (std::is_same<ResourceType, Font>::value) {
            auto& map = getFontMap();
            auto it = map.find(key);
            if (it == map.end()) {
				// Load automatically if missing
                if (loadFont(key)) {
                    it = map.find(key);
                    if (it != map.end()) return *it->second;
                }

                Logger::Get().Error(DebugType::ResourceManager)
                    << "[ResourceManager] Missing Font: \"" << key << "\"\n";
                throw std::runtime_error("Missing Font: " + key);
            }
            return *it->second;
        }

        // AnimationInfo
        else if constexpr (std::is_same<ResourceType, AnimationInfo>::value) {
            auto& map = getAnimationMap();
            auto it = map.find(key);
            if (it == map.end()) {
				// Load automatically if missing
                if (loadAnimationInfo(key)) {
                    it = map.find(key);
                    if (it != map.end()) return *it->second;
                }

                Logger::Get().Error(DebugType::ResourceManager)
                    << "[ResourceManager] Missing Animation: \"" << key << "\"\n";
                throw std::runtime_error("Missing Animation: " + key);
            }
            return *it->second;
        }

        // Unsupported type
        else {
            static_assert(sizeof(ResourceType) == 0,
                "ResourceManager::get only supports Texture, Audio, Font, and AnimationInfo.");
        }
    }

    // Copyable resources
    template<typename ResourceType>
    void ResourceManager::add(const std::string& name, ResourceType resource) {
        if constexpr (std::is_same<ResourceType, Texture>::value) {
            auto& map = getTextureMap();
			auto it = map.find(name);
			// If key already exists, do not overwrite, instead replace
            if (it != map.end()) {
                // Replace existing texture
                it->second.reset(new Texture(resource));
            }

			// If key does not exist, create new entry
            else {
                // NOTE: Must use 'new Texture(resource)' to pass a raw pointer 
                // to unique_ptr's constructor, which uses TextureDeleter implicitly.
                map.emplace(name, std::unique_ptr<Texture, TextureDeleter>(new Texture(resource)));
            }
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
            auto it = map.find(name);

            if (it != map.end()) {
                it->second = std::move(resourcePtr);
            }
            else {
                map.emplace(name, std::move(resourcePtr));
            }
        }
        else if constexpr (std::is_same<ResourceType, Font>::value) {
            auto& map = getFontMap();
            auto it = map.find(name);

            if (it != map.end()) {
                it->second = std::move(resourcePtr);
            }
            else {
                map.emplace(name, std::move(resourcePtr));
            }
        }   
        else if constexpr (std::is_same<ResourceType, AnimationInfo>::value) {
            auto& map = getAnimationMap();
            auto it = map.find(name);

            if (it != map.end()) {
                it->second = std::move(resourcePtr);
            }
            else {
                map.emplace(name, std::move(resourcePtr));
            }
        }
        else {
            static_assert(sizeof(ResourceType) == 0,
                "add() unique_ptr version only supports Audio/Font/Animation at the moment.");
        }
    }

    template<typename ResourceType>
    bool ResourceManager::has(const std::string& key) const {
        // Texture
        if constexpr (std::is_same<ResourceType, Texture>::value) {
            return textureMap.find(key) != textureMap.end();
        }
        // Audio
        else if constexpr (std::is_same<ResourceType, Audio>::value) {
            return soundMap.find(key) != soundMap.end();
        }
        // Font
        else if constexpr (std::is_same<ResourceType, Font>::value) {
            return fontMap.find(key) != fontMap.end();
        }
        // AnimationInfo
        else if constexpr (std::is_same<ResourceType, AnimationInfo>::value) {
            return animationMap.find(key) != animationMap.end();
        }
        // Unsupported type
        else {
            static_assert(sizeof(ResourceType) == 0,
                "ResourceManager::has only supports Texture, Audio, Font, and AnimationInfo.");
        }
        return false;
    }

} // namespace Framework
