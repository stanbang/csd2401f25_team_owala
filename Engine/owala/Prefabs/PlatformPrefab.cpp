#include "Platform.h"
#include "TrapPlatform.h"
#include "ScalingPlatform.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <PlatformPrefab.h>
#include "EntityManager.h"

namespace Framework {

    void PrefabSystem::RegisterPlatformPrefab(const PlatformPrefab& prefab) {
        platformPrefabs[prefab.name] = prefab;
    }

    void PrefabSystem::RegisterTrapPlatformPrefab(const TrapPlatformPrefab& prefab) {
        trapPlatformPrefabs[prefab.name] = prefab;
    }

    void PrefabSystem::RegisterScalingPlatformPrefab(const ScalingPlatformPrefab& prefab) {
        scalingPlatformPrefabs[prefab.name] = prefab;
    }

    Entity PrefabSystem::CreatePlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position) {
        auto it = platformPrefabs.find(prefabName);
        if (it == platformPrefabs.end()) {
            std::cerr << "Platform prefab not found: " << prefabName << std::endl;
            return 0;
        }

        const PlatformPrefab& prefab = it->second;
        Entity platform = createPlatform(storage, position, prefab.size, prefab.textureID);
        EntityManager entityManager{storage};

        if (entityManager.HasComponent(platform, RigidBodyType)) {
            entityManager.GetComponent<RigidBody>(platform).SetMass(prefab.mass);
        }

        return platform;
    }

    Entity PrefabSystem::CreateTrapPlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position) {
        auto it = trapPlatformPrefabs.find(prefabName);
        if (it == trapPlatformPrefabs.end()) {
            std::cerr << "Trap platform prefab not found: " << prefabName << std::endl;
            return 0;
        }

        const TrapPlatformPrefab& prefab = it->second;

        Entity trapPlatform = createPlatform(storage, position, prefab.size, prefab.textureID);

        TrapPlatformComponent trapComp;
        trapComp.originalPosition = position;
        trapComp.isTriggered = false;
        trapComp.hasFallen = false;
        trapComp.currentDelay = 0.0f;
        trapComp.currentFallDistance = 0.0f;
        trapComp.fallSpeed = prefab.fallSpeed;
        trapComp.triggerDelay = prefab.triggerDelay;

        EntityManager entityManager{storage};
        entityManager.AddComponent(trapPlatform, trapComp);

        if (entityManager.HasComponent(trapPlatform, RigidBodyType)) {
            entityManager.GetComponent<RigidBody>(trapPlatform).SetMass(prefab.mass);
        }

        return trapPlatform;
    }

    Entity PrefabSystem::CreateScalingPlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position) {
        auto it = scalingPlatformPrefabs.find(prefabName);
        if (it == scalingPlatformPrefabs.end()) {
            std::cerr << "Scaling platform prefab not found: " << prefabName << std::endl;
            return 0;
        }
        const ScalingPlatformPrefab& prefab = it->second;

        Entity scalingPlatformEntity = createScalingPlatform(storage,position,prefab.size,prefab.textureID,prefab.scaleSpeed,prefab.minScale, prefab.maxScale);
        return scalingPlatformEntity;
    }

    bool PrefabSystem::LoadPrefabsFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to open prefab file: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream iss(line);
            std::string type, name, textureID;
            float sizeX, sizeY;

            if (!(iss >> type >> name >> textureID >> sizeX >> sizeY)) {
                std::cerr << "Invalid prefab line: " << line << std::endl;
                continue;
            }

            Vector2D size(sizeX, sizeY);

            if (type == "PLATFORM") {
                PlatformPrefab prefab(name, size, textureID);
                if (!(iss >> prefab.mass)) {
                    prefab.mass = 0.0f;
                }
                std::cout << "[PARSING] Platform - Mass: " << prefab.mass << std::endl;
                RegisterPlatformPrefab(prefab);
                std::cout << "Registered platform prefab: " << name << std::endl;
            }
            else if (type == "TRAP_PLATFORM") {
                TrapPlatformPrefab prefab(name, size, textureID);
                if (!(iss >> prefab.triggerDelay >> prefab.fallSpeed)) {
                    std::cerr << "Invalid trap platform parameters: " << line << std::endl;
                    continue;
                }
                std::cout << "[PARSING] Trap Platform - Trigger Delay: " << prefab.triggerDelay << ", Fall Speed: " << prefab.fallSpeed << std::endl;

                RegisterTrapPlatformPrefab(prefab);
                std::cout << "Registered trap platform prefab: " << name << std::endl;
            }
            else if (type == "SCALING_PLATFORM") {
                ScalingPlatformPrefab prefab(name, size, textureID);
                if (!(iss >> prefab.scaleSpeed >> prefab.minScale >> prefab.maxScale)) {
                    std::cerr << "Invalid scaling platform parameters: " << line << std::endl;
                    continue;
                }
                std::cout << "[PARSING] Scaling Platform - Scale Speed: " << prefab.scaleSpeed << ", Min: " << prefab.minScale << ", Max: " << prefab.maxScale << std::endl;

                RegisterScalingPlatformPrefab(prefab);
                std::cout << "Registered scaling platform prefab: " << name << std::endl;
            }
            else {
                std::cerr << "Unknown prefab type: " << type << std::endl;
            }
        }

        file.close();
        return true;
    }

    bool PrefabSystem::HasPlatformPrefab(const std::string& name) const {
        return platformPrefabs.find(name) != platformPrefabs.end();
    }

    bool PrefabSystem::HasTrapPlatformPrefab(const std::string& name) const {
        return trapPlatformPrefabs.find(name) != trapPlatformPrefabs.end();
    }

    bool PrefabSystem::HasScalingPlatformPrefab(const std::string& name) const {
        return scalingPlatformPrefabs.find(name) != scalingPlatformPrefabs.end();
    }

    const PlatformPrefab* PrefabSystem::GetPlatformPrefab(const std::string& name) const {
        auto it = platformPrefabs.find(name);
        return (it != platformPrefabs.end()) ? &it->second : nullptr;
    }

    const TrapPlatformPrefab* PrefabSystem::GetTrapPlatformPrefab(const std::string& name) const {
        auto it = trapPlatformPrefabs.find(name);
        return (it != trapPlatformPrefabs.end()) ? &it->second : nullptr;
    }

    const ScalingPlatformPrefab* PrefabSystem::GetScalingPlatformPrefab(const std::string& name) const {
        auto it = scalingPlatformPrefabs.find(name);
        return (it != scalingPlatformPrefabs.end()) ? &it->second : nullptr;
    }

} // namespace Framework