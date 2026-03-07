/*****************************************************************//**
 * \file   RopeSystem.cpp
 *
 * \brief  Rope attachment for vertical and horizontal trap platforms.
 *         Vertical platforms: 2 ropes (left and right sides)
 *         Horizontal platforms: 1 rope (top center)
 *         MultiAxis platforms: NO ropes
 *
 *         Ropes are fixed at the nearest tiles above/beside the
 *         platform's original position and stretch/compress as the
 *         platform moves.
 *********************************************************************/
#include "pch.h"
#include "RopeSystem.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "RopeComponent.h"
#include "TransformationSystem.h"
 //#include "Components.h"
#include "Logger.h"

namespace {
    bool HasTwoRopes(Framework::EntityManager& entityMgr, Framework::Entity platform) {
        if (!entityMgr.HasComponent(platform, Framework::TextureComponentType)) return false;
        const Framework::TextureComponent& tex =
            entityMgr.GetComponent<Framework::TextureComponent>(platform);
        return tex.textureKey == "SB_TrapPlatform_Vertical.png";
    }

    bool FallsVertically(const std::string& platformName) {
        return platformName.find("Vertical") != std::string::npos;
    }
} 

namespace Framework {


    void RopeSystem::Initialize() {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        std::vector<Entity> staleRopes;
        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            staleRopes.push_back(rope);
        }
        for (Entity rope : staleRopes) {
            entityMgr.DestroyEntity(rope);
        }

        InitializeRopesForAllPlatforms();
        Logger::Get().Info(DebugType::Unknown) << "RopeSystem initialized" << std::endl;
    }

    void RopeSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        static int frameCount = 0;
        if (++frameCount % 60 == 0) { 
            int ropeCount = 0;
            for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
                ropeCount++;
            }
            Logger::Get().Info(DebugType::Unknown)
                << "RopeSystem Update - Current rope count: " << ropeCount << std::endl;
        }

        std::vector<Entity> ropesToDestroy;
        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            RopeComponent& rc = entityMgr.GetComponent<RopeComponent>(rope);

            Logger::Get().Info(DebugType::Unknown)
                << "Rope " << rope << " attached to platform: " << rc.attachedPlatform
                << " (valid: " << entityMgr.ValidateEntity(rc.attachedPlatform) << ")" << std::endl;

            if (rc.attachedPlatform == 0 || !entityMgr.ValidateEntity(rc.attachedPlatform)) {
                const std::string& ropeName = entityMgr.GetProperties(rope).name;
                Logger::Get().Info(DebugType::Unknown)
                    << "Rope has invalid platform, searching by name: " << ropeName << std::endl;

                std::string platformName = ropeName;
                size_t ropePos = platformName.find("_Rope");
                if (ropePos != std::string::npos) {
                    platformName = platformName.substr(0, ropePos);
                }

                for (const Entity& platform : entityMgr.GetAllEntitiesWithComponent(false, TrapPlatformComponentType)) {
                    if (entityMgr.GetProperties(platform).name == platformName) {
                        Logger::Get().Info(DebugType::Unknown)
                            << "Found matching platform: " << platformName << " (ID: " << platform << ")" << std::endl;
                        rc.attachedPlatform = platform;
                        break;
                    }
                }
            }
        }

        // Pass 2: ENSURE ROPES EXIST - but only for platforms that need them
        for (const Entity& platform : entityMgr.GetAllEntitiesWithComponent(false, TrapPlatformComponentType)) {
            const std::string& name = entityMgr.GetProperties(platform).name;
            Logger::Get().Info(DebugType::Unknown)
                << "Checking platform: " << name << " for ropes" << std::endl;

            if (IsValidRopePlatform(name)) {
                // Count existing ropes for this platform
                int existingRopes = 0;
                for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
                    RopeComponent& rc = entityMgr.GetComponent<RopeComponent>(rope);
                    if (rc.attachedPlatform == platform) {
                        existingRopes++;
                    }
                }

                // Rope count driven by texture key only.
                int requiredRopes = HasTwoRopes(entityMgr, platform) ? 2 : 1;

                Logger::Get().Info(DebugType::Unknown)
                    << "Platform " << name << " has " << existingRopes
                    << "/" << requiredRopes << " ropes" << std::endl;

                // Create missing ropes if needed
                if (existingRopes < requiredRopes) {
                    Logger::Get().Info(DebugType::Unknown)
                        << "Creating missing ropes for platform: " << name << std::endl;
                    AttachRopeToPlatform(platform);
                }
            }
        }

        // Pass 3: Update rope transforms
        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            RopeComponent& rc = entityMgr.GetComponent<RopeComponent>(rope);
            if (!rc.isActive) continue;

            if (rc.attachedPlatform == 0 || !entityMgr.ValidateEntity(rc.attachedPlatform)) {
                Logger::Get().Info(DebugType::Unknown)
                    << "Skipping rope " << rope << " with invalid platform" << std::endl;
                continue;
            }

            UpdateRopeTransform(rope, rc.attachedPlatform);
        }
    }

    void RopeSystem::Shutdown() {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        std::vector<Entity> ropesToDelete;
        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            ropesToDelete.push_back(rope);
        }

        for (Entity rope : ropesToDelete) {
            entityMgr.DestroyEntity(rope);
        }

        Logger::Get().Info(DebugType::Unknown) << "RopeSystem shutdown, cleared all ropes" << std::endl;
    }


    void RopeSystem::InitializeRopesForAllPlatforms() {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        int ropeCount = 0;
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(false, TrapPlatformComponentType)) {
            const std::string& name = entityMgr.GetProperties(entity).name;
            if (IsValidRopePlatform(name)) {
                AttachRopeToPlatform(entity);
                ropeCount++;
            }
        }

        Logger::Get().Info(DebugType::Unknown)
            << "Initialized ropes for " << ropeCount << " platforms" << std::endl;
    }

    void RopeSystem::AttachRopeToPlatform(Entity platform) {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        if (!entityMgr.HasComponent(platform, TransformType) ||
            !entityMgr.HasComponent(platform, TrapPlatformComponentType)) {
            Logger::Get().Info(DebugType::Unknown)
                << "Platform missing required components for ropes" << std::endl;
            return;
        }

        const std::string& platformName = entityMgr.GetProperties(platform).name;
        Logger::Get().Info(DebugType::Unknown)
            << "AttachRopeToPlatform called for: " << platformName << std::endl;

        // Skip if not a valid rope platform
        if (!IsValidRopePlatform(platformName)) {
            Logger::Get().Info(DebugType::Unknown)
                << "Platform " << platformName << " is not a valid rope platform" << std::endl;
            return;
        }

        // Get platform dimensions
        float halfWidth = 40.0f;
        float halfHeight = 20.0f;
        if (entityMgr.HasComponent(platform, AABBColliderType)) {
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(platform);
            halfWidth = col.HalfSize.x;
            halfHeight = col.HalfSize.y;
            Logger::Get().Info(DebugType::Unknown)
                << "Platform dimensions: " << halfWidth << " x " << halfHeight << std::endl;
        }

        if (entityMgr.HasComponent(platform, TransformType)) {
            float colAngle = entityMgr.GetComponent<Transform>(platform).rotation.colliderAngle;
            float absRot = std::fmod(std::abs(colAngle), 3.14159265f);
            if (absRot > 3.14159265f * 0.25f && absRot < 3.14159265f * 0.75f) {
                std::swap(halfWidth, halfHeight);
            }
        }

        const TrapPlatformComponent& trapComp =
            entityMgr.GetComponent<TrapPlatformComponent>(platform);

        const Vector2D originalPos = entityMgr.GetComponent<Transform>(platform).position;

        if (trapComp.originalPosition.x == 0.0f && trapComp.originalPosition.y == 0.0f) {
            entityMgr.GetComponent<TrapPlatformComponent>(platform).originalPosition = originalPos;
        }
        Logger::Get().Info(DebugType::Unknown)
            << "Original position: (" << originalPos.x << ", " << originalPos.y << ")" << std::endl;

        int numRopes = HasTwoRopes(entityMgr, platform) ? 2 : 1;

        // Track which rope indices already exist
        std::vector<bool> existingIndices(numRopes, false);

        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            RopeComponent& rc = entityMgr.GetComponent<RopeComponent>(rope);
            if (rc.attachedPlatform == platform && rc.ropeIndex < numRopes) {
                existingIndices[rc.ropeIndex] = true;
            }
        }

        // Create missing ropes
        for (int idx = 0; idx < numRopes; ++idx) {
            if (!existingIndices[idx]) {
                Logger::Get().Info(DebugType::Unknown)
                    << "Creating rope index " << idx << " for platform " << platformName << std::endl;
                CreateSingleRope(platform, platformName, idx, originalPos, halfWidth, halfHeight);
            }
        }
    }

    void RopeSystem::RemoveRopeFromPlatform(Entity platform) {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        std::vector<Entity> toRemove;
        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            if (entityMgr.GetComponent<RopeComponent>(rope).attachedPlatform == platform) {
                toRemove.push_back(rope);
            }
        }

        for (Entity rope : toRemove) {
            entityMgr.DestroyEntity(rope);
        }
    }

    bool RopeSystem::HasRopesAttached(Entity platform) const {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        for (const Entity& rope : entityMgr.GetAllEntitiesWithComponent(false, RopeComponentType)) {
            if (entityMgr.GetComponent<RopeComponent>(rope).attachedPlatform == platform) {
                return true;
            }
        }
        return false;
    }


    bool RopeSystem::IsValidRopePlatform(const std::string& platformName) const {
        bool isValid = (platformName.find("Vertical") != std::string::npos ||
            platformName.find("Horizontal") != std::string::npos) &&
            platformName.find("MultiAxis") == std::string::npos;

        return isValid;
    }

    Vector2D RopeSystem::CalculateFixedAnchor(const Vector2D& originalPosition,
        const std::string& platformType,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const {
        Vector2D anchor = originalPosition;

        if (FallsVertically(platformType)) {
            // Vertical platform: anchors above the left and right top corners
            if (ropeIndex == 0) { // Left rope
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

    Vector2D RopeSystem::CalculateDynamicAnchor(const Vector2D& currentPosition,
        const std::string& platformType,
        float halfWidth,
        float halfHeight,
        int ropeIndex) const {
        Vector2D anchor = currentPosition;

        if (FallsVertically(platformType)) {
            if (ropeIndex == 0) { // Left rope
                anchor.x = currentPosition.x - halfWidth;
                anchor.y = currentPosition.y + halfHeight;
            }
            else { // Right rope
                anchor.x = currentPosition.x + halfWidth;
                anchor.y = currentPosition.y + halfHeight;
            }
        }
        else {
            anchor.x = currentPosition.x - halfWidth;
            anchor.y = currentPosition.y; // vertical centre
        }

        return anchor;
    }

    static float FindCeilingAnchorY(EntityManager& entityMgr,
        float attachX,
        float minY,
        float fallbackY)
    {
        float best = fallbackY;
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, PlatformTagType)) {
            // Exclude trap platforms (they fall away and aren't real ceilings)
            if (entityMgr.HasComponent(e, TrapPlatformComponentType)) continue;
            if (!entityMgr.HasComponent(e, TransformType) ||
                !entityMgr.HasComponent(e, AABBColliderType)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float left = tf.position.x - col.HalfSize.x;
            float right = tf.position.x + col.HalfSize.x;
            float bottom = tf.position.y - col.HalfSize.y; // bottom edge of this tile

            // Small tolerance on X overlap so corner-attached ropes still match
            constexpr float kXTol = 5.0f;
            if (bottom > minY &&
                attachX >= (left - kXTol) &&
                attachX <= (right + kXTol))
            {
                if (bottom < best) best = bottom; // closest ceiling wins
            }
        }
        return best;
    }

    static float FindWallAnchorX(EntityManager& entityMgr,
        float attachY,
        float maxX,
        float fallbackX)
    {
        float best = fallbackX;
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(false, PlatformTagType)) {
            if (entityMgr.HasComponent(e, TrapPlatformComponentType)) continue;
            if (!entityMgr.HasComponent(e, TransformType) ||
                !entityMgr.HasComponent(e, AABBColliderType)) continue;

            const Transform& tf = entityMgr.GetComponent<Transform>(e);
            const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(e);

            float top = tf.position.y + col.HalfSize.y;
            float bottom = tf.position.y - col.HalfSize.y;
            float right = tf.position.x + col.HalfSize.x; // right edge of this tile

            constexpr float kYTol = 5.0f;
            if (right < maxX &&
                attachY >= (bottom - kYTol) &&
                attachY <= (top + kYTol))
            {
                if (right > best) best = right; // closest wall to the right wins
            }
        }
        return best;
    }

    void RopeSystem::CreateSingleRope(Entity platform,
        const std::string& platformName,
        int ropeIndex,
        const Vector2D& originalPos,
        float halfWidth,
        float halfHeight) {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        Logger::Get().Info(DebugType::Unknown)
            << "CreateSingleRope - Platform: " << platformName
            << ", Index: " << ropeIndex << std::endl;


        Vector2D dynamicAnchor;
        if (FallsVertically(platformName)) {
            bool twoRopes = HasTwoRopes(entityMgr, platform);
            dynamicAnchor.x = originalPos.x + (twoRopes ? ((ropeIndex == 0) ? -halfWidth : halfWidth) : 0.0f);
            dynamicAnchor.y = originalPos.y + halfHeight;

            if (entityMgr.HasComponent(platform, AABBColliderType)) {
                const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(platform);
                float colAngle = entityMgr.GetComponent<Transform>(platform).rotation.colliderAngle;
                float cosA = std::cos(colAngle);
                float sinA = std::sin(colAngle);
                dynamicAnchor.x += col.Offset.x * cosA - col.Offset.y * sinA;
                dynamicAnchor.y += col.Offset.x * sinA + col.Offset.y * cosA;
            }
        }
        else {
            // Left-centre edge in world space.
            dynamicAnchor.x = originalPos.x - halfWidth;
            dynamicAnchor.y = originalPos.y;
        }

        Vector2D fixedAnchor;

        if (FallsVertically(platformName)) {
            float attachX = dynamicAnchor.x;
            float attachTopY = dynamicAnchor.y;
            float fallbackCeilY = attachTopY + ROPE_OVERHEAD;
            float ceilY = FindCeilingAnchorY(entityMgr, attachX, attachTopY, fallbackCeilY);
            fixedAnchor = Vector2D(attachX, ceilY);
        }
        else {
            // Search for the nearest static wall tile to the left of the attachment point.
            float attachX = dynamicAnchor.x;
            float attachY = dynamicAnchor.y;
            float fallbackWallX = attachX - ROPE_OVERHEAD;
            float wallX = FindWallAnchorX(entityMgr, attachY, attachX, fallbackWallX);
            fixedAnchor = Vector2D(wallX, attachY);
        }

        Logger::Get().Info(DebugType::Unknown)
            << "Fixed anchor: (" << fixedAnchor.x << ", " << fixedAnchor.y << ")"
            << "  Dynamic anchor: (" << dynamicAnchor.x << ", " << dynamicAnchor.y << ")" << std::endl;

        // Calculate initial rope length
        Vector2D delta = dynamicAnchor - fixedAnchor;
        float length = delta.Magnitude();
        Logger::Get().Info(DebugType::Unknown) << "Rope length: " << length << std::endl;

        // Don't create if length is too small
        if (length < MIN_ROPE_LENGTH) {
            length = MIN_ROPE_LENGTH;
        }

        // Create rope entity
        Entity rope = entityMgr.CreateEntity();
        Logger::Get().Info(DebugType::Unknown) << "Created rope entity ID: " << rope << std::endl;

        // Set properties
        EntityProperties& props = entityMgr.GetProperties(rope);
        props.name = platformName + (ropeIndex == 0 ? "_RopeL" : "_RopeR");
        props.renderLayer = RenderLayer::Platform;
        props.layer = CollisionLayer::Default;
        props.isActive = true;  // Ensure it's active

        // Required tags
        entityMgr.AddComponent(rope, LoadedTag{});

        // Mirror platform's SceneOwned
        if (entityMgr.HasComponent(platform, SceneOwnedType)) {
            entityMgr.AddComponent(rope, entityMgr.GetComponent<SceneOwned>(platform));
        }

        // RopeComponent
        RopeComponent rc;
        rc.attachedPlatform = platform;
        rc.textureKey = "SB_PlatformRope.png";
        rc.isActive = true;
        rc.segmentHeight = 20.0f;
        rc.ropeIndex = ropeIndex;
        rc.ceilingAttachmentPoint = fixedAnchor;
        rc.platformAttachmentPoint = dynamicAnchor;
        rc.originalLength = length;
        rc.currentLength = length;
        rc.ropeWidth = 200.0f; 
        rc.hasFoundCeiling = true;

        entityMgr.AddComponent(rope, rc);

        
        Transform ropeTf;
        ropeTf.position = (fixedAnchor + dynamicAnchor) * 0.5f;
        float angleRad = atan2(delta.x, -delta.y);
        ropeTf.rotation.angle = angleRad * (180.0f / 3.14159265f); 
        ropeTf.rotation.textureAngle = angleRad;
        entityMgr.AddComponent(rope, ropeTf);

        TextureComponent texComp;
        texComp.textureKey = rc.textureKey;
        texComp.HalfSize = Vector2D(rc.ropeWidth * 0.5f, length * 0.5f);
        texComp.flipHorizontal = false;
        texComp.alpha = 1.0f;
        entityMgr.AddComponent(rope, texComp);

        RotationFlag ropeRotFlag;
        ropeRotFlag.isRotating = false;
        ropeRotFlag.currentRotation = 0.0f;
        ropeRotFlag.targetRotation = 0.0f;
        entityMgr.AddComponent(rope, ropeRotFlag);

        Logger::Get().Info(DebugType::Unknown)
            << "Rope created successfully - Position: (" << ropeTf.position.x
            << ", " << ropeTf.position.y << "), Length: " << length << std::endl;
    }

    void RopeSystem::UpdateRopeTransform(Entity rope, Entity platform) {
        GameManager& gm = GameManager::GetInstance();
        EntityManager entityMgr(gm.GetECS());

        if (!entityMgr.HasComponent(rope, RopeComponentType) ||
            !entityMgr.HasComponent(rope, TransformType) ||
            !entityMgr.HasComponent(rope, TextureComponentType)) {
            return;
        }
        if (!entityMgr.HasComponent(platform, TrapPlatformComponentType)) {
            return;
        }

        RopeComponent& rc = entityMgr.GetComponent<RopeComponent>(rope);
        Transform& ropeTf = entityMgr.GetComponent<Transform>(rope);
        TextureComponent& texComp = entityMgr.GetComponent<TextureComponent>(rope);
        const std::string& platformName = entityMgr.GetProperties(platform).name;
        const Transform& platformTf = entityMgr.GetComponent<Transform>(platform);

        float halfWidth = 40.0f, halfHeight = 20.0f;
        if (entityMgr.HasComponent(platform, AABBColliderType)) {
            halfWidth = entityMgr.GetComponent<AABBCollider>(platform).HalfSize.x;
            halfHeight = entityMgr.GetComponent<AABBCollider>(platform).HalfSize.y;
        }

        {
            float colAngle = entityMgr.GetComponent<Transform>(platform).rotation.colliderAngle;
            float absRot = std::fmod(std::abs(colAngle), 3.14159265f);
            if (absRot > 3.14159265f * 0.25f && absRot < 3.14159265f * 0.75f) {
                std::swap(halfWidth, halfHeight);
            }
        }

        if (!rc.hasFoundCeiling) {
            Logger::Get().Info(DebugType::Unknown)
                << "Rope " << rope << " WARNING: hasFoundCeiling was false — "
                << "recomputing anchor. This should not happen during normal gameplay." << std::endl;

            if (FallsVertically(platformName)) {
                float attachX = platformTf.position.x + ((rc.ropeIndex == 0) ? -halfWidth : halfWidth);
                float platformTopY = platformTf.position.y + halfHeight;
                float fallbackCeilY = platformTopY + ROPE_OVERHEAD;
                rc.ceilingAttachmentPoint.x = attachX;
                rc.ceilingAttachmentPoint.y = FindCeilingAnchorY(entityMgr, attachX, platformTopY, fallbackCeilY);
            }
            else {
                float attachY = platformTf.position.y;
                float platformLeftX = platformTf.position.x - halfWidth;
                float fallbackWallX = platformLeftX - ROPE_OVERHEAD;
                rc.ceilingAttachmentPoint.x = FindWallAnchorX(entityMgr, attachY, platformLeftX, fallbackWallX);
                rc.ceilingAttachmentPoint.y = attachY;
            }

            rc.hasFoundCeiling = true;
        }

        float worldRotDeg = 0.0f;
        if (entityMgr.HasComponent(platform, RotationFlagType)) {
            worldRotDeg = entityMgr.GetComponent<RotationFlag>(platform).currentRotation;
        }
        const float worldRotRad = worldRotDeg * (3.14159265f / 180.0f);
        const float cosR = std::cos(worldRotRad);
        const float sinR = std::sin(worldRotRad);

        Vector2D localEdge;
        if (FallsVertically(platformName)) {
            bool twoRopes = HasTwoRopes(entityMgr, platform);
            localEdge.x = twoRopes ? ((rc.ropeIndex == 0) ? -halfWidth : halfWidth) : 0.0f;
            localEdge.y = halfHeight;

            if (entityMgr.HasComponent(platform, AABBColliderType)) {
                const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(platform);
                float colAngle = entityMgr.GetComponent<Transform>(platform).rotation.colliderAngle;
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

        const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR,
            localEdge.x * sinR + localEdge.y * cosR);
        rc.platformAttachmentPoint = platformTf.position + worldEdge;

        const Vector2D& worldCeiling = rc.ceilingAttachmentPoint;
        const Vector2D delta = rc.platformAttachmentPoint - worldCeiling;
        rc.currentLength = delta.Magnitude();

        if (rc.currentLength < MIN_ROPE_LENGTH) {
            rc.currentLength = MIN_ROPE_LENGTH;
        }

        ropeTf.position = (worldCeiling + rc.platformAttachmentPoint) * 0.5f;

        const float angleRad = std::atan2(delta.x, -delta.y);
        const float angleDeg = angleRad * (180.0f / 3.14159265f);

        ropeTf.rotation.textureAngle = 0.0f;
        ropeTf.rotation.angle = angleDeg;

        if (entityMgr.HasComponent(rope, RotationFlagType)) {
            entityMgr.GetComponent<RotationFlag>(rope).currentRotation = angleDeg;
        }

        texComp.HalfSize.x = rc.ropeWidth * 0.5f;
        texComp.HalfSize.y = rc.currentLength * 0.5f;
        ropeTf.scale = Vector2D(1.0f, 1.0f);

        if (gm.GetSystems().transformation_) {
            gm.GetSystems().transformation_->UpdateEntityTransform(rope);
        }
    }

} // namespace Framework