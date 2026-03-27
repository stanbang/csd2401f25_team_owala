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
#include <unordered_map>

using namespace Framework;

class RopeScript : public ScriptableEntity {
public:
    static constexpr float MIN_ROPE_LENGTH = 20.0f;
    static constexpr float ROPE_OVERHEAD = 20.0f;
    static constexpr float HORIZONTAL_FALLBACK_LENGTH = 160.0f;

protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;

private:
    struct RopeRuntimeData {
        Entity attachedPlatform = 0;
        Entity renderEntity = 0;
        std::string textureKey = "SB_PlatformRope.png";
        float originalLength = 0.0f;
        float currentLength = 0.0f;
        Vector2D ceilingAttachmentPoint = { 0.0f, 0.0f };
        Vector2D platformAttachmentPoint = { 0.0f, 0.0f };
        bool isActive = true;
        float ropeWidth = 200.0f;
        float selectionHalfWidth = 7.0f;
        float segmentHeight = 20.0f;
        int ropeIndex = 0;
        bool hasFoundCeiling = true;
        float lastWorldRotationDeg = 0.0f;
    };

    // Helper methods
    EntityManager* GetEntityManager() const;
    RopeRuntimeData* FindRopeRuntime(Entity rope) const;
    void DestroyTrackedRope(EntityManager& entityMgr, Entity rope, Entity renderRope) const;
    void SyncRenderRope(EntityManager& entityMgr, const RopeRuntimeData& runtimeData,
        const Transform& proxyTransform, float angleDeg) const;
    bool HasTwoRopes(EntityManager& entityMgr) const;
    bool IsValidRopePlatform() const;
    bool FallsVertically() const;
    bool UsesCeilingTopRope(EntityManager& entityMgr) const;
    Vector2D CalculateTopColliderAttachment(EntityManager& entityMgr, int ropeIndex) const;

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
    mutable std::unordered_map<Entity, RopeRuntimeData> ropeRuntimeData_;
    bool hasInitialized_ = false;
    std::string platformType_;
    float originalHalfWidth_ = 40.0f;
    float originalHalfHeight_ = 20.0f;
    Vector2D originalPosition_;
};
