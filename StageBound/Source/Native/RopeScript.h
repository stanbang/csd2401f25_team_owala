/*****************************************************************//**
 * \file   RopeScript.h
 *
 * \primary_author Sitara Anil
 * 
 * \secondary_author
 *
 *
 * \brief  NativeScript implementation for rope attachment to platforms.
 *         Vertical platforms: 2 ropes (left and right sides)
 *         Horizontal platforms: 1 rope (top center)
 *         MultiAxis platforms: NO ropes
 *
 *         Ropes are fixed at the nearest tiles above/beside the
 *         platform's original position and stretch/compress as the
 *         platform moves.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"
#include "Components.h"
#include "EntityManager.h"
#include <vector>
#include <string>

 // Use the Framework namespace for types, but class is in global namespace
using namespace Framework;

class RopeScript : public ScriptableEntity {
public:
    static constexpr float MIN_ROPE_LENGTH = 20.0f;
    static constexpr float ROPE_OVERHEAD = 20.0f;

protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;

private:
    // Helper methods
    EntityManager* GetEntityManager() const;
    bool HasTwoRopes(EntityManager& entityMgr) const;
    bool IsValidRopePlatform() const;
    bool FallsVertically() const;

    void InitializeRopesForPlatform();
    void RemoveAllRopes();
    void CreateSingleRope(int ropeIndex);
    void UpdateRopeTransform(Entity rope) const;

    Vector2D CalculateFixedAnchor(const Vector2D& originalPosition,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const;

    Vector2D CalculateDynamicAnchor(const Vector2D& currentPosition,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const;

    float FindCeilingAnchorY(EntityManager& entityMgr, float attachX, float minY, float fallbackY) const;
    float FindWallAnchorX(EntityManager& entityMgr, float attachY, float maxX, float fallbackX) const;

    Entity platformEntity_ = 0;
    std::vector<Entity> ropeEntities_;
    bool hasInitialized_ = false;
    std::string platformType_;
    float originalHalfWidth_ = 40.0f;
    float originalHalfHeight_ = 20.0f;
    Vector2D originalPosition_;
};