/*****************************************************************//**
* \file   PlatformPrefab.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "Entity.h"
#include "EntityStorage.h"
#include "Vector2D.h"
#include <string>
#include <unordered_map>

namespace Framework {

    struct PlatformPrefab {
        std::string name;
        Vector2D size;
        std::string textureID;
        bool isStatic = true;
        float mass{0.0f};
        PlatformPrefab() : name(""), size(Vector2D(0, 0)), textureID("") {}
        PlatformPrefab(const std::string& name, const Vector2D& size, const std::string& textureID): name(name), size(size), textureID(textureID) {}
    };


    struct TrapPlatformPrefab : public PlatformPrefab {
        float triggerDelay{0.0f};
        float fallSpeed{0.0f};
        TrapPlatformPrefab() : PlatformPrefab() {}
        TrapPlatformPrefab(const std::string& name, const Vector2D& size, const std::string& textureID) : PlatformPrefab(name, size, textureID) { isStatic = false; }
    };

    struct ScalingPlatformPrefab : public PlatformPrefab {
        float scaleSpeed{0.0f};
        float minScale{0.0f};
        float maxScale{0.0f};
        ScalingPlatformPrefab() : PlatformPrefab() {}
        ScalingPlatformPrefab(const std::string& name, const Vector2D& size, const std::string& textureID): PlatformPrefab(name, size, textureID) {}
    };

    class PrefabSystem {
    private:
        std::unordered_map<std::string, PlatformPrefab> platformPrefabs;
        std::unordered_map<std::string, TrapPlatformPrefab> trapPlatformPrefabs;
        std::unordered_map<std::string, ScalingPlatformPrefab> scalingPlatformPrefabs;

    public:
        void RegisterPlatformPrefab(const PlatformPrefab& prefab);
        void RegisterTrapPlatformPrefab(const TrapPlatformPrefab& prefab);
        void RegisterScalingPlatformPrefab(const ScalingPlatformPrefab& prefab);

        Entity CreatePlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position);
        Entity CreateTrapPlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position);
        Entity CreateScalingPlatformFromPrefab(EntityStorage& storage, const std::string& prefabName, const Vector2D& position);

        bool LoadPrefabsFromFile(const std::string& filename);

        bool HasPlatformPrefab(const std::string& name) const;
        bool HasTrapPlatformPrefab(const std::string& name) const;
        bool HasScalingPlatformPrefab(const std::string& name) const;

        const PlatformPrefab* GetPlatformPrefab(const std::string& name) const;
        const TrapPlatformPrefab* GetTrapPlatformPrefab(const std::string& name) const;
        const ScalingPlatformPrefab* GetScalingPlatformPrefab(const std::string& name) const;
    };

} // namespace Framework