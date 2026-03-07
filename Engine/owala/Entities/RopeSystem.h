/*****************************************************************//**
 * \file   RopeSystem.h
 * \brief  System for managing rope behavior on trap platforms.
 *
 *  Vertical platforms  → 2 ropes (left end + right end)
 *  Horizontal platforms → 1 rope (top center)
 *  MultiAxis platforms → NO ropes
 *
 *  The ceiling anchor is set ONCE when the rope is created and never
 *  moves. Only the platform-end attachment point updates each frame,
 *  so the rope stretches as the platform falls away.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "Entity.h"
#include "Vector2D.h"

namespace Framework {
    class GameManager;
    class TransformationSystem;
}

namespace Framework {
    class RopeSystem : public ISystem {
    private:
        static constexpr float ROPE_OVERHEAD = 300.0f; 
        static constexpr float MIN_ROPE_LENGTH = 10.0f;

        bool IsValidRopePlatform(const std::string& platformName) const;

        Vector2D CalculateFixedAnchor(const Vector2D& originalPosition,
            const std::string& platformType,
            float halfWidth,
            float halfHeight,
            int ropeIndex) const;

        Vector2D CalculateDynamicAnchor(const Vector2D& currentPosition,
            const std::string& platformType,
            float halfWidth,
            float halfHeight,
            int ropeIndex) const;

        void CreateSingleRope(Entity platform,
            const std::string& platformName,
            int ropeIndex,
            const Vector2D& originalPos,
            float halfWidth,
            float halfHeight);

        void UpdateRopeTransform(Entity rope, Entity platform);

    public:
        RopeSystem() = default;
        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        bool HasRopesAttached(Entity platform) const;

        void InitializeRopesForAllPlatforms();

        void AttachRopeToPlatform(Entity platform);

        void RemoveRopeFromPlatform(Entity platform);
    };
}