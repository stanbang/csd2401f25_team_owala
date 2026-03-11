/*****************************************************************//**
 * \file   RopeScript.cpp
 *
 * \primary_author Sitara Anil
 * \secondary_author
 *
 *
 * \brief  NativeScript implementation for rope attachment to platforms.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "RopeScript.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "TransformationSystem.h"
#include "Logger.h"
#include "SceneManager.h"

REGISTER_SCRIPT(RopeScript, "RopeScript")

// ---------------------------------------------------------------------------
// Lifecycle Methods
// ---------------------------------------------------------------------------
void RopeScript::OnCreate() {
    platformEntity_ = GetEntity();
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    // Store platform type
    platformType_ = entityMgr->GetProperties(platformEntity_).name;

    Logger::Get().Info(DebugType::Unknown)
        << "RopeScript::OnCreate for platform: " << platformType_ << std::endl;

    // Check if this platform should have ropes
    if (!IsValidRopePlatform()) {
        return;
    }

    // Check if platform has required components
    if (!entityMgr->HasComponent(platformEntity_, TransformType) ||
        !entityMgr->HasComponent(platformEntity_, TrapPlatformComponentType)) {
        return;
    }

    // Cache platform dimensions
    if (entityMgr->HasComponent(platformEntity_, AABBColliderType)) {
        const AABBCollider& col = entityMgr->GetComponent<AABBCollider>(platformEntity_);
        originalHalfWidth_ = col.HalfSize.x;
        originalHalfHeight_ = col.HalfSize.y;
    }

    // Cache original position
    const Transform& transform = entityMgr->GetComponent<Transform>(platformEntity_);
    originalPosition_ = transform.position;

    // Store original position in TrapPlatformComponent if needed
    TrapPlatformComponent& trapComp = entityMgr->GetComponent<TrapPlatformComponent>(platformEntity_);
    if (trapComp.originalPosition.x == 0.0f && trapComp.originalPosition.y == 0.0f) {
        trapComp.originalPosition = originalPosition_;
    }

    // Handle collider rotation swapping
    float colAngle = transform.rotation.colliderAngle;
    float absRot = std::fmod(std::abs(colAngle), 3.14159265f);
    if (absRot > 3.14159265f * 0.25f && absRot < 3.14159265f * 0.75f) {
        std::swap(originalHalfWidth_, originalHalfHeight_);
    }

    // Initialize ropes for this platform
    InitializeRopesForPlatform();
    hasInitialized_ = true;
}

void RopeScript::OnUpdate() {
    if (!hasInitialized_) return;

    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    // Skip if platform no longer exists or is invalid
    if (!entityMgr->ValidateEntity(platformEntity_)) {
        RemoveAllRopes();
        return;
    }    // Check if platform still has required components
    if (!entityMgr->HasComponent(platformEntity_, TrapPlatformComponentType) ||
        !entityMgr->HasComponent(platformEntity_, TransformType)) {
        return;
    }

    // Clean up invalid ropes
    for (auto it = ropeEntities_.begin(); it != ropeEntities_.end();) {
        Entity rope = *it;
        if (!entityMgr->ValidateEntity(rope)) {
            it = ropeEntities_.erase(it);
        }
        else {
            ++it;
        }
    }

    // Check if we need to create missing ropes
    int requiredRopes = HasTwoRopes(*entityMgr) ? 2 : 1;
    if (static_cast<int>(ropeEntities_.size()) < requiredRopes) {
        InitializeRopesForPlatform();
    }

    // Update all rope transforms
    for (Entity rope : ropeEntities_) {
        UpdateRopeTransform(rope);
    }
}

void RopeScript::OnDestroy() {
    Logger::Get().Info(DebugType::Unknown) << "RopeScript::OnDestroy for platform" << std::endl;
    RemoveAllRopes();
}

// ---------------------------------------------------------------------------
// Private Helper Methods
// ---------------------------------------------------------------------------
EntityManager* RopeScript::GetEntityManager() const {
    if (!GameManager::Exists()) return nullptr;

    // Obtain reference to current EntityStorage from GameManager
    EntityStorage& ecs = GameManager::GetInstance().GetECS();

    // Cache an EntityManager that wraps the current ECS reference. If the underlying
    // ECS instance changes (e.g. editor play/stop swapping storages), recreate the
    // cached EntityManager to point to the new storage.
    static EntityStorage* cachedEcs = nullptr;
    static std::unique_ptr<EntityManager> cachedManager;

    if (cachedEcs != &ecs) {
        cachedEcs = &ecs;
        cachedManager.reset(new EntityManager(ecs));
    }

    return cachedManager.get();
}

bool RopeScript::HasTwoRopes(EntityManager& entityMgr) const {
    if (!entityMgr.HasComponent(platformEntity_, TextureComponentType)) return false;
    const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(platformEntity_);
    return tex.textureKey == "SB_TrapPlatform_Vertical.png";
}

bool RopeScript::IsValidRopePlatform() const {
    bool isValid = (platformType_.find("Vertical") != std::string::npos ||
        platformType_.find("Horizontal") != std::string::npos) &&
        platformType_.find("MultiAxis") == std::string::npos;
    return isValid;
}

bool RopeScript::FallsVertically() const {
    return platformType_.find("Vertical") != std::string::npos;
}

void RopeScript::InitializeRopesForPlatform() {
    // Remove any existing ropes first
    RemoveAllRopes();

    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    int numRopes = HasTwoRopes(*entityMgr) ? 2 : 1;

    for (int idx = 0; idx < numRopes; ++idx) {
        CreateSingleRope(idx);
    }
}

void RopeScript::RemoveAllRopes() {
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    for (const Entity& rope : entityMgr->GetAllEntitiesWithComponent(false, RopeComponentType)) {
        RopeComponent& rc = entityMgr->GetComponent<RopeComponent>(rope);
        if (rc.attachedPlatform == platformEntity_) {
            entityMgr->DestroyEntity(rope);
        }
    }

    for (Entity rope : ropeEntities_) {
        if (entityMgr->ValidateEntity(rope)) {
            entityMgr->DestroyEntity(rope);
        }
    }
    ropeEntities_.clear();
}

void RopeScript::CreateSingleRope(int ropeIndex) {
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    // Get current platform transform
    const Transform& platformTf = entityMgr->GetComponent<Transform>(platformEntity_);
    float halfWidth = originalHalfWidth_;
    float halfHeight = originalHalfHeight_;

    // Handle rotation if platform has rotated
    float worldRotDeg = 0.0f;
    if (entityMgr->HasComponent(platformEntity_, RotationFlagType)) {
        worldRotDeg = entityMgr->GetComponent<RotationFlag>(platformEntity_).currentRotation;
    }
    const float worldRotRad = worldRotDeg * (3.14159265f / 180.0f);
    const float cosR = std::cos(worldRotRad);
    const float sinR = std::sin(worldRotRad);

    Vector2D localEdge;
    if (FallsVertically()) {
        bool twoRopes = HasTwoRopes(*entityMgr);
        localEdge.x = twoRopes ? ((ropeIndex == 0) ? -halfWidth : halfWidth) : 0.0f;
        localEdge.y = halfHeight;

        if (entityMgr->HasComponent(platformEntity_, AABBColliderType)) {
            const AABBCollider& col = entityMgr->GetComponent<AABBCollider>(platformEntity_);
            float colAngle = entityMgr->GetComponent<Transform>(platformEntity_).rotation.colliderAngle;
            float cosA = std::cos(colAngle);
            float sinA = std::sin(colAngle);
            localEdge.x += col.Offset.x * cosA - col.Offset.y * sinA;
            localEdge.y += col.Offset.x * sinA + col.Offset.y * cosA;
        }
    }
    else {
        localEdge.x = -halfWidth;
        localEdge.y = 0.0f;
    }

    // Transform to world space
    const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR,
        localEdge.x * sinR + localEdge.y * cosR);
    Vector2D dynamicAnchor = platformTf.position + worldEdge;

    // Calculate fixed anchor
    Vector2D fixedAnchor;
    if (FallsVertically()) {
        float attachX = dynamicAnchor.x;
        float attachTopY = dynamicAnchor.y;
        float fallbackCeilY = attachTopY + ROPE_OVERHEAD;
        float ceilY = FindCeilingAnchorY(*entityMgr, attachX, attachTopY, fallbackCeilY);
        fixedAnchor = Vector2D(attachX, ceilY);
    }
    else {
        float attachX = dynamicAnchor.x;
        float attachY = dynamicAnchor.y;
        float fallbackWallX = attachX - ROPE_OVERHEAD;
        float wallX = FindWallAnchorX(*entityMgr, attachY, attachX, fallbackWallX);
        fixedAnchor = Vector2D(wallX, attachY);
    }

    Vector2D delta = dynamicAnchor - fixedAnchor;
    float length = delta.Magnitude();

    if (length < MIN_ROPE_LENGTH) {
        length = MIN_ROPE_LENGTH;
    }

    // Create rope entity
    Entity rope = entityMgr->CreateEntity();
    Logger::Get().Info(DebugType::Unknown) << "Created rope entity ID: " << rope << std::endl;

    // Set properties
    EntityProperties& props = entityMgr->GetProperties(rope);
    props.name = platformType_ + (ropeIndex == 0 ? "_RopeL" : "_RopeR");
    props.renderLayer = RenderLayer::Platform;
    props.layer = CollisionLayer::Default;
    props.isActive = true;

    entityMgr->AddComponent(rope, LoadedTag{});

    if (entityMgr->HasComponent(platformEntity_, SceneOwnedType)) {
        entityMgr->AddComponent(rope, entityMgr->GetComponent<SceneOwned>(platformEntity_));
    }

    // RopeComponent
    RopeComponent rc;
    rc.attachedPlatform = platformEntity_;
    rc.textureKey = "SB_PlatformRope.png";
    rc.isActive = true;
    rc.segmentHeight = 20.0f;
    rc.ropeWidth = 200.0f;
    rc.ropeIndex = ropeIndex;
    rc.ceilingAttachmentPoint = fixedAnchor;
    rc.platformAttachmentPoint = dynamicAnchor;
    rc.originalLength = length;
    rc.currentLength = length;
    rc.hasFoundCeiling = true;

    entityMgr->AddComponent(rope, rc);

    // Transform
    Transform ropeTf;
    ropeTf.position = (fixedAnchor + dynamicAnchor) * 0.5f;
    float angleRad = atan2(delta.x, -delta.y);
    ropeTf.rotation.angle = angleRad * (180.0f / 3.14159265f);
    ropeTf.rotation.textureAngle = angleRad;
    entityMgr->AddComponent(rope, ropeTf);

    // Texture component
    TextureComponent texComp;
    texComp.textureKey = rc.textureKey;
    texComp.HalfSize = Vector2D(rc.ropeWidth * 0.5f, length * 0.5f);
    texComp.flipHorizontal = false;
    texComp.alpha = 1.0f;
    entityMgr->AddComponent(rope, texComp);

    // Rotation flag
    RotationFlag ropeRotFlag;
    ropeRotFlag.isRotating = false;
    ropeRotFlag.currentRotation = 0.0f;
    ropeRotFlag.targetRotation = 0.0f;
    entityMgr->AddComponent(rope, ropeRotFlag);

    // Store rope entity
    ropeEntities_.push_back(rope);

    Logger::Get().Info(DebugType::Unknown)
        << "Rope created successfully - Position: (" << ropeTf.position.x
        << ", " << ropeTf.position.y << "), Length: " << length << std::endl;
}

void RopeScript::UpdateRopeTransform(Entity rope) const {
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    if (!entityMgr->HasComponent(rope, RopeComponentType) ||
        !entityMgr->HasComponent(rope, TransformType) ||
        !entityMgr->HasComponent(rope, TextureComponentType)) {
        return;
    }

    RopeComponent& rc = entityMgr->GetComponent<RopeComponent>(rope);
    Transform& ropeTf = entityMgr->GetComponent<Transform>(rope);
    TextureComponent& texComp = entityMgr->GetComponent<TextureComponent>(rope);
    const Transform& platformTf = entityMgr->GetComponent<Transform>(platformEntity_);

    float halfWidth = originalHalfWidth_;
    float halfHeight = originalHalfHeight_;

    // Handle platform rotation
    float worldRotDeg = 0.0f;
    if (entityMgr->HasComponent(platformEntity_, RotationFlagType)) {
        worldRotDeg = entityMgr->GetComponent<RotationFlag>(platformEntity_).currentRotation;
    }
    const float worldRotRad = worldRotDeg * (3.14159265f / 180.0f);
    const float cosR = std::cos(worldRotRad);
    const float sinR = std::sin(worldRotRad);

    // Handle collider rotation swapping
    {
        float colAngle = entityMgr->GetComponent<Transform>(platformEntity_).rotation.colliderAngle;
        float absRot = std::fmod(std::abs(colAngle), 3.14159265f);
        if (absRot > 3.14159265f * 0.25f && absRot < 3.14159265f * 0.75f) {
            std::swap(halfWidth, halfHeight);
        }
    }

    // Recalculate ceiling if needed
    if (!rc.hasFoundCeiling) {
        if (FallsVertically()) {
            float attachX = platformTf.position.x + ((rc.ropeIndex == 0) ? -halfWidth : halfWidth);
            float platformTopY = platformTf.position.y + halfHeight;
            float fallbackCeilY = platformTopY + ROPE_OVERHEAD;
            rc.ceilingAttachmentPoint.x = attachX;
            rc.ceilingAttachmentPoint.y = FindCeilingAnchorY(*entityMgr, attachX, platformTopY, fallbackCeilY);
        }
        else {
            float attachY = platformTf.position.y;
            float platformLeftX = platformTf.position.x - halfWidth;
            float fallbackWallX = platformLeftX - ROPE_OVERHEAD;
            rc.ceilingAttachmentPoint.x = FindWallAnchorX(*entityMgr, attachY, platformLeftX, fallbackWallX);
            rc.ceilingAttachmentPoint.y = attachY;
        }

        rc.hasFoundCeiling = true;
    }

    // Calculate attachment point in local space
    Vector2D localEdge;
    if (FallsVertically()) {
        bool twoRopes = HasTwoRopes(*entityMgr);
        localEdge.x = twoRopes ? ((rc.ropeIndex == 0) ? -halfWidth : halfWidth) : 0.0f;
        localEdge.y = halfHeight;

        if (entityMgr->HasComponent(platformEntity_, AABBColliderType)) {
            const AABBCollider& col = entityMgr->GetComponent<AABBCollider>(platformEntity_);
            float colAngle = entityMgr->GetComponent<Transform>(platformEntity_).rotation.colliderAngle;
            float cosA = std::cos(colAngle);
            float sinA = std::sin(colAngle);
            localEdge.x += col.Offset.x * cosA - col.Offset.y * sinA;
            localEdge.y += col.Offset.x * sinA + col.Offset.y * cosA;
        }
    }
    else {
        localEdge.x = -halfWidth;
        localEdge.y = 0.0f;
    }

    // Transform to world space
    const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR, localEdge.x * sinR + localEdge.y * cosR);
    rc.platformAttachmentPoint = platformTf.position + worldEdge;

    // Update rope length
    const Vector2D& worldCeiling = rc.ceilingAttachmentPoint;
    const Vector2D delta = rc.platformAttachmentPoint - worldCeiling;
    rc.currentLength = delta.Magnitude();

    if (rc.currentLength < MIN_ROPE_LENGTH) {
        rc.currentLength = MIN_ROPE_LENGTH;
    }

    // Update rope transform
    ropeTf.position = (worldCeiling + rc.platformAttachmentPoint) * 0.5f;

    const float angleRad = std::atan2(delta.x, -delta.y);
    const float angleDeg = angleRad * (180.0f / 3.14159265f);

    ropeTf.rotation.textureAngle = 0.0f;
    ropeTf.rotation.angle = angleDeg;

    if (entityMgr->HasComponent(rope, RotationFlagType)) {
        entityMgr->GetComponent<RotationFlag>(rope).currentRotation = angleDeg;
    }

    // Update texture size
    texComp.HalfSize.x = rc.ropeWidth * 0.5f;
    texComp.HalfSize.y = rc.currentLength * 0.5f;
    ropeTf.scale = Vector2D(1.0f, 1.0f);

    // Update transformation system
    if (GameManager::Exists() && GameManager::GetInstance().GetSystems().transformation_) {
        GameManager::GetInstance().GetSystems().transformation_->UpdateEntityTransform(rope);
    }
}

float RopeScript::FindCeilingAnchorY(EntityManager& entityMgr, float attachX, float minY, float fallbackY) const {
    // First pass: Check platforms
    {
        float bestBottom = fallbackY;
        bool found = false;

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, PlatformTagType)) {
            if (entityMgr.HasComponent(e, TrapPlatformComponentType)) continue;
            if (!entityMgr.HasComponent(e, TransformType) ||
                !entityMgr.HasComponent(e, AABBColliderType)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float colCentreX = tf.position.x + col.Offset.x;
            float colCentreY = tf.position.y + col.Offset.y;
            float left = colCentreX - col.HalfSize.x;
            float right = colCentreX + col.HalfSize.x;
            float bottom = colCentreY - col.HalfSize.y;

            constexpr float kXTol = 5.0f;
            if (attachX < (left - kXTol) || attachX >(right + kXTol)) continue;
            if (bottom <= minY) continue;

            if (!found || bottom < bestBottom) {
                bestBottom = bottom;
                found = true;
            }
        }

        if (found) return bestBottom;
    }

    // Second pass: Check all colliders
    {
        float bestBottom = fallbackY;
        bool found = false;

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, AABBColliderType)) {
            if (!entityMgr.HasComponent(e, TransformType)) continue;

            // Skip unwanted entity types
            if (entityMgr.HasComponent(e, TrapPlatformComponentType)) continue;
            if (entityMgr.HasComponent(e, PlatformTagType)) continue;
            if (entityMgr.HasComponent(e, PlayerTagType)) continue;
            if (entityMgr.HasComponent(e, EnemyTagType)) continue;
            if (entityMgr.HasComponent(e, GUITagType)) continue;
            if (entityMgr.HasComponent(e, RopeComponentType)) continue;
            if (entityMgr.HasComponent(e, SpawnPointTagType)) continue;
            if (entityMgr.HasComponent(e, ButtonType)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float colCentreX = tf.position.x + col.Offset.x;
            float colCentreY = tf.position.y + col.Offset.y;
            float left = colCentreX - col.HalfSize.x;
            float right = colCentreX + col.HalfSize.x;

            float bottom;
            if (entityMgr.HasComponent(e, TextureComponentType)) {
                const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(e);
                bottom = tf.position.y - tex.HalfSize.y;
            }
            else {
                bottom = colCentreY - col.HalfSize.y;
            }

            constexpr float kXTol = 20.0f;
            if (attachX < (left - kXTol) || attachX >(right + kXTol)) continue;
            constexpr float kYTol = 2.0f;
            if ((bottom + kYTol) <= minY) continue;

            if (!found || bottom < bestBottom) {
                bestBottom = bottom;
                found = true;
            }
        }

        if (found) return bestBottom;
    }

    return fallbackY;
}

float RopeScript::FindWallAnchorX(EntityManager& entityMgr, float attachY, float maxX, float fallbackX) const {
    auto GetWorldAABB = [&](const Transform& tf, const AABBCollider& col,
        float& outRight, float& outTop, float& outBottom) {
            float colAngle = tf.rotation.colliderAngle;
            float cosA = std::abs(std::cos(colAngle));
            float sinA = std::abs(std::sin(colAngle));

            float hw = col.HalfSize.x;
            float hh = col.HalfSize.y;

            float worldHalfX = hw * cosA + hh * sinA;
            float worldHalfY = hw * sinA + hh * cosA;

            float offX = col.Offset.x * std::cos(colAngle) - col.Offset.y * std::sin(colAngle);
            float offY = col.Offset.x * std::sin(colAngle) + col.Offset.y * std::cos(colAngle);

            float centreX = tf.position.x + offX;
            float centreY = tf.position.y + offY;

            outRight = centreX + worldHalfX;
            outTop = centreY + worldHalfY;
            outBottom = centreY - worldHalfY;
        };

    auto ShouldSkip = [&](const Entity& e) -> bool {
        if (entityMgr.HasComponent(e, TrapPlatformComponentType)) return true;
        if (entityMgr.HasComponent(e, PlayerTagType))             return true;
        if (entityMgr.HasComponent(e, EnemyTagType))              return true;
        if (entityMgr.HasComponent(e, GUITagType))                return true;
        if (entityMgr.HasComponent(e, RopeComponentType))         return true;
        if (entityMgr.HasComponent(e, SpawnPointTagType))         return true;
        if (entityMgr.HasComponent(e, ButtonType))                return true;
        return false;
        };

    {
        float bestRight = fallbackX;
        bool  found = false;

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, AABBColliderType)) {
            if (!entityMgr.HasComponent(e, TransformType)) continue;
            if (ShouldSkip(e)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float right, top, bottom;
            GetWorldAABB(tf, col, right, top, bottom);

            // Wall must end at or before the platform's left edge
            if (right > maxX + 6.0f) continue;

            constexpr float kYTol = 5.0f;
            if (attachY < (bottom - kYTol) || attachY >(top + kYTol)) continue;

            if (!found || right > bestRight) {
                bestRight = right;
                found = true;
            }
        }

        if (found) return bestRight;
    }

    {
        float bestRight = fallbackX;
        float bestHeight = 0.0f;
        bool  found = false;

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, AABBColliderType)) {
            if (!entityMgr.HasComponent(e, TransformType)) continue;
            if (ShouldSkip(e)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float right, top, bottom;
            GetWorldAABB(tf, col, right, top, bottom);
            float height = top - bottom;

            if (right > maxX + 6.0f) continue;

            bool isBetter = !found
                || right > bestRight
                || (std::abs(right - bestRight) < 1.0f && height > bestHeight);

            if (isBetter) {
                bestRight = right;
                bestHeight = height;
                found = true;
            }
        }

        if (found) return bestRight;
    }

    return fallbackX;
}

Vector2D RopeScript::CalculateFixedAnchor(const Vector2D& originalPosition,
    float halfWidth,
    float halfHeight,
    int ropeIndex) const {
    Vector2D anchor = originalPosition;

    if (FallsVertically()) {
        if (ropeIndex == 0) {
            anchor.x = originalPosition.x - halfWidth;
            anchor.y = originalPosition.y + halfHeight + ROPE_OVERHEAD;
        }
        else { // Right rope
            anchor.x = originalPosition.x + halfWidth;
            anchor.y = originalPosition.y + halfHeight + ROPE_OVERHEAD;
        }
    }
    else {
        anchor.x = originalPosition.x - halfWidth - ROPE_OVERHEAD;
        anchor.y = originalPosition.y;
    }

    return anchor;
}

Vector2D RopeScript::CalculateDynamicAnchor(const Vector2D& currentPosition,
    float halfWidth,
    float halfHeight,
    int ropeIndex) const {
    Vector2D anchor = currentPosition;

    if (FallsVertically()) {
        if (ropeIndex == 0) {
            anchor.x = currentPosition.x - halfWidth;
            anchor.y = currentPosition.y + halfHeight;
        }
        else {
            anchor.x = currentPosition.x + halfWidth;
            anchor.y = currentPosition.y + halfHeight;
        }
    }
    else {
        anchor.x = currentPosition.x - halfWidth;
        anchor.y = currentPosition.y;
    }

    return anchor;
}