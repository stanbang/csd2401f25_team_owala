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
#include "FileUtility.h"

REGISTER_SCRIPT(RopeScript, "RopeScript")
namespace {
    constexpr float kRopeCeilingVisualOverlap = 15.0f;
    constexpr float kRopeWallVisualTrim = 4.0f;
    constexpr float kHiddenRopeLengthThreshold = 2.0f;

    struct RopePrefabData {
        std::string textureKey{};
        float halfWidth = 0.0f;
        float selectionHalfWidth = 0.0f;
        std::string collisionLayerName{};
        std::string renderLayerName{};
        int collisionLayer = -1;
        int renderOrder = 0;
        bool isLoaded = false;
    };

    const RopePrefabData& GetRopePrefabData() {
        static RopePrefabData data;
        static bool attemptedLoad = false;
        if (attemptedLoad) return data;
        attemptedLoad = true;

        rapidjson::Document doc;
        const char* paths[] = {
            "StageBound/Data/Prefabs/TrapPlatforms/PlatformRope.json",
            "Data/Prefabs/TrapPlatforms/PlatformRope.json"
        };

        for (const char* path : paths) {
            if (!FileIOSystem::ReadFile(path, doc) || !doc.IsObject()) {
                continue;
            }

            RopePrefabData loadedData;

            if (doc.HasMember("components") && doc["components"].IsObject()) {
                const auto& comps = doc["components"];

                if (comps.HasMember("TextureComponent") && comps["TextureComponent"].IsObject()) {
                    const auto& tex = comps["TextureComponent"];
                    if (tex.HasMember("texture_key") && tex["texture_key"].IsString()) {
                        loadedData.textureKey = tex["texture_key"].GetString();
                    }
                    if (tex.HasMember("half_size") && tex["half_size"].IsObject()) {
                        const auto& hs = tex["half_size"];
                        if (hs.HasMember("x") && hs["x"].IsNumber()) {
                            loadedData.halfWidth = hs["x"].GetFloat();
                        }
                    }
                }

                if (comps.HasMember("AABBCollider") && comps["AABBCollider"].IsObject()) {
                    const auto& collider = comps["AABBCollider"];
                    if (collider.HasMember("half_width") && collider["half_width"].IsNumber()) {
                        loadedData.selectionHalfWidth = collider["half_width"].GetFloat();
                    }
                }
            }

            if (doc.HasMember("layer") && doc["layer"].IsInt()) {
                loadedData.collisionLayer = doc["layer"].GetInt();
            }
            if (doc.HasMember("layer_collision_name") && doc["layer_collision_name"].IsString()) {
                loadedData.collisionLayerName = doc["layer_collision_name"].GetString();
            }
            if (doc.HasMember("custom_render_order") && doc["custom_render_order"].IsInt()) {
                loadedData.renderOrder = doc["custom_render_order"].GetInt();
            }
            if (doc.HasMember("layer_render_name") && doc["layer_render_name"].IsString()) {
                loadedData.renderLayerName = doc["layer_render_name"].GetString();
            }

            if (loadedData.selectionHalfWidth <= 0.0f && loadedData.halfWidth > 0.0f) {
                loadedData.selectionHalfWidth = loadedData.halfWidth;
            }

            rapidjson::Document layerDoc;
            const char* layerPaths[] = {
                "StageBound/Data/LayerConfig.json",
                "Data/LayerConfig.json"
            };

            for (const char* layerPath : layerPaths) {
                if (!FileIOSystem::ReadFile(layerPath, layerDoc) || !layerDoc.IsObject()) {
                    continue;
                }

                if (layerDoc.HasMember("CollisionLayers") && layerDoc["CollisionLayers"].IsArray()) {
                    for (const auto& layer : layerDoc["CollisionLayers"].GetArray()) {
                        if (!layer.IsObject()) continue;
                        if (!layer.HasMember("name") || !layer["name"].IsString()) continue;
                        if (!layer.HasMember("id") || !layer["id"].IsInt()) continue;

                        if (loadedData.collisionLayerName == layer["name"].GetString()) {
                            loadedData.collisionLayer = layer["id"].GetInt();
                            break;
                        }
                    }
                }

                if (layerDoc.HasMember("RenderLayers") && layerDoc["RenderLayers"].IsArray()) {
                    for (const auto& layer : layerDoc["RenderLayers"].GetArray()) {
                        if (!layer.IsObject()) continue;
                        if (!layer.HasMember("name") || !layer["name"].IsString()) continue;
                        if (!layer.HasMember("renderOrder") || !layer["renderOrder"].IsInt()) continue;

                        if (loadedData.renderLayerName == layer["name"].GetString()) {
                            loadedData.renderOrder = layer["renderOrder"].GetInt();
                            break;
                        }
                    }
                }
                break;
            }

            loadedData.isLoaded =
                !loadedData.textureKey.empty() &&
                loadedData.halfWidth > 0.0f &&
                loadedData.selectionHalfWidth > 0.0f &&
                loadedData.collisionLayer >= 0;

            data = loadedData;
            break;
        }

        if (!data.isLoaded) {
            Logger::Get().Error(DebugType::Unknown)
                << "RopeScript: Failed to load complete rope prefab data from PlatformRope.json."
                << std::endl;
        }

        return data;
    }

    Vector2D GetCeilingRenderOffset(const Vector2D& delta, float extraLength) {
        const float ropeLength = delta.Magnitude();
        if (ropeLength <= 0.001f || extraLength <= 0.0f) {
            return Vector2D::Zero();
        }

        return (-delta / ropeLength) * (extraLength * 0.5f);
    }

    float GetPlatformColliderLeftEdge(EntityManager& entityMgr, Entity platformEntity, const Transform& platformTf, float fallbackLeftEdge) {
        if (!entityMgr.HasComponent(platformEntity, AABBColliderType)) {
            return fallbackLeftEdge;
        }

        const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(platformEntity);
        const float colAngle = platformTf.rotation.colliderAngle;
        const float cosA = std::abs(std::cos(colAngle));
        const float sinA = std::abs(std::sin(colAngle));

        const float worldHalfX = col.HalfSize.x * cosA + col.HalfSize.y * sinA;
        const float offX = col.Offset.x * std::cos(colAngle) - col.Offset.y * std::sin(colAngle);
        const float centreX = platformTf.position.x + offX;
        return centreX - worldHalfX;
    }
}

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
            ropeRuntimeData_.erase(rope);
            it = ropeEntities_.erase(it);
        }
        else {
            auto runtimeIt = ropeRuntimeData_.find(rope);
            const bool missingRuntime = runtimeIt == ropeRuntimeData_.end();
            const bool missingRenderEntity = !missingRuntime &&
                runtimeIt->second.renderEntity != 0 &&
                !entityMgr->ValidateEntity(runtimeIt->second.renderEntity);

            if (missingRuntime || missingRenderEntity) {
                if (!missingRuntime && runtimeIt->second.renderEntity != 0 &&
                    entityMgr->ValidateEntity(runtimeIt->second.renderEntity)) {
                    entityMgr->DestroyEntity(runtimeIt->second.renderEntity);
                }

                if (entityMgr->ValidateEntity(rope)) {
                    entityMgr->DestroyEntity(rope);
                }

                ropeRuntimeData_.erase(rope);
                it = ropeEntities_.erase(it);
            }
            else {
                ++it;
            }
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
    RemoveAllRopes();
}

// ---------------------------------------------------------------------------
// Private Helper Methods
// ---------------------------------------------------------------------------
EntityManager* RopeScript::GetEntityManager() const {
    if (!GameManager::Exists()) return nullptr;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();

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

bool RopeScript::UsesCeilingTopRope(EntityManager& entityMgr) const {
    if (FallsVertically()) {
        return true;
    }

    if (!entityMgr.HasComponent(platformEntity_, TextureComponentType)) {
        return false;
    }

    const TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(platformEntity_);
    return tex.textureKey == "SB_MovableBox_02.png";
}

Vector2D RopeScript::CalculateTopColliderAttachment(EntityManager& entityMgr, int ropeIndex) const {
    const Transform& platformTf = entityMgr.GetComponent<Transform>(platformEntity_);

    float halfWidth = originalHalfWidth_;
    float halfHeight = originalHalfHeight_;

    const float colAngle = platformTf.rotation.colliderAngle;
    const float absRot = std::fmod(std::abs(colAngle), 3.14159265f);
    if (absRot > 3.14159265f * 0.25f && absRot < 3.14159265f * 0.75f) {
        std::swap(halfWidth, halfHeight);
    }

    Vector2D localEdge = Vector2D::Zero();
    const bool twoRopes = HasTwoRopes(entityMgr);
    if (FallsVertically()) {
        localEdge.x = twoRopes ? ((ropeIndex == 0) ? -halfWidth : halfWidth) : 0.0f;
        localEdge.y = halfHeight;
    }
    else {
        localEdge.x = -halfWidth;
        localEdge.y = 0.0f;
    }

    if (entityMgr.HasComponent(platformEntity_, AABBColliderType)) {
        const AABBCollider& col = entityMgr.GetComponent<AABBCollider>(platformEntity_);
        const float cosA = std::cos(colAngle);
        const float sinA = std::sin(colAngle);
        localEdge.x += col.Offset.x * cosA - col.Offset.y * sinA;
        localEdge.y += col.Offset.x * sinA + col.Offset.y * cosA;
    }

    float worldRotDeg = 0.0f;
    if (entityMgr.HasComponent(platformEntity_, RotationFlagType)) {
        worldRotDeg = entityMgr.GetComponent<RotationFlag>(platformEntity_).currentRotation;
    }

    const float worldRotRad = worldRotDeg * (3.14159265f / 180.0f);
    const float cosR = std::cos(worldRotRad);
    const float sinR = std::sin(worldRotRad);
    const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR, localEdge.x * sinR + localEdge.y * cosR);
    return platformTf.position + worldEdge;
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

    const std::string ropeNamePrefix = "_" + std::to_string(platformEntity_) + "_Rope";
    std::vector<Entity> ropesToDestroy;
    for (const Entity& e : entityMgr->GetAllEntities()) {
        if (!entityMgr->ValidateEntity(e)) continue;
        const EntityProperties& props = entityMgr->GetProperties(e);
        if (props.name.find(ropeNamePrefix) != std::string::npos) {
            ropesToDestroy.push_back(e);
        }
    }

    for (Entity rope : ropesToDestroy) {
        if (entityMgr->ValidateEntity(rope)) {
            entityMgr->DestroyEntity(rope);
        }
    }

    for (Entity rope : ropeEntities_) {
        if (entityMgr->ValidateEntity(rope)) {
            entityMgr->DestroyEntity(rope);
        }
    }

    ropeEntities_.clear();
    ropeRuntimeData_.clear();
}
void RopeScript::CreateSingleRope(int ropeIndex) {
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    // Get current platform transform
    const Transform& platformTf = entityMgr->GetComponent<Transform>(platformEntity_);
    float halfWidth = originalHalfWidth_;
    // float halfHeight = originalHalfHeight_;

     // Handle rotation if platform has rotated
    float worldRotDeg = 0.0f;
    if (entityMgr->HasComponent(platformEntity_, RotationFlagType)) {
        worldRotDeg = entityMgr->GetComponent<RotationFlag>(platformEntity_).currentRotation;
    }
    const float worldRotRad = worldRotDeg * (3.14159265f / 180.0f);
    const float cosR = std::cos(worldRotRad);
    const float sinR = std::sin(worldRotRad);
    const bool useSpecialPlatformAttachment = UsesCeilingTopRope(*entityMgr);
    const bool useCeilingAnchor = FallsVertically();

    Vector2D localEdge = Vector2D::Zero();
    if (!useSpecialPlatformAttachment) {
        localEdge.x = -halfWidth;
        localEdge.y = 0.0f;
    }

    // Transform to world space
    const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR, localEdge.x * sinR + localEdge.y * cosR);
    Vector2D dynamicAnchor = useSpecialPlatformAttachment ? CalculateTopColliderAttachment(*entityMgr, ropeIndex) : platformTf.position + worldEdge;

    // Calculate fixed anchor
    Vector2D fixedAnchor;
    if (useCeilingAnchor) {
        float attachX = dynamicAnchor.x;
        float attachTopY = dynamicAnchor.y;
        float fallbackCeilY = attachTopY + ROPE_OVERHEAD;
        float ceilY = FindCeilingAnchorY(*entityMgr, attachX, attachTopY, fallbackCeilY);
        fixedAnchor = Vector2D(attachX, ceilY);
    }
    else {
        float attachX = dynamicAnchor.x;
        float attachY = dynamicAnchor.y;
        float fallbackWallX = attachX - HORIZONTAL_FALLBACK_LENGTH;
        float wallX = FindWallAnchorX(*entityMgr, attachY, attachX, fallbackWallX);
        fixedAnchor = Vector2D(wallX, attachY);
    }

    Vector2D delta = dynamicAnchor - fixedAnchor;
    const float rawLength = delta.Magnitude();
    float length = rawLength;

    if (length < MIN_ROPE_LENGTH) {
        length = MIN_ROPE_LENGTH;
    }

    // Set properties from PlatformRope prefab data
    const RopePrefabData& ropePrefab = GetRopePrefabData();
    if (!ropePrefab.isLoaded) {
        return;
    }

    // Create rope entity
    Entity rope = entityMgr->CreateEntity();
    Logger::Get().Info(DebugType::Unknown) << "Created rope entity ID: " << rope << std::endl;

    const std::string ropeName = platformType_ + "_" + std::to_string(platformEntity_) +
        (ropeIndex == 0 ? "_RopeL" : "_RopeR");

    auto ApplyRopeProperties = [&](Entity entity, const std::string& name) {
        EntityProperties& props = entityMgr->GetProperties(entity);
        props.name = name;
        props.renderLayer = RenderLayer::Custom;
        props.customRenderOrder = ropePrefab.renderOrder;
        props.renderLayerName = ropePrefab.renderLayerName;
        props.layer = static_cast<CollisionLayer>(ropePrefab.collisionLayer);
        props.collisionLayerName = ropePrefab.collisionLayerName;
        props.isActive = true;
        };

    ApplyRopeProperties(rope, ropeName);

    entityMgr->AddComponent(rope, LoadedTag{});

    if (entityMgr->HasComponent(platformEntity_, SceneOwnedType)) {
        entityMgr->AddComponent(rope, entityMgr->GetComponent<SceneOwned>(platformEntity_));
    }

    // Runtime rope data stored in RopeScript
    RopeRuntimeData rc;
    rc.attachedPlatform = platformEntity_;
    rc.textureKey = ropePrefab.textureKey;
    rc.isActive = true;
    rc.segmentHeight = 20.0f;
    rc.ropeWidth = ropePrefab.halfWidth * 2.0f;
    rc.selectionHalfWidth = ropePrefab.selectionHalfWidth > 0.0f ? std::min(ropePrefab.selectionHalfWidth, ropePrefab.halfWidth) : ropePrefab.halfWidth;
    rc.ropeIndex = ropeIndex;
    rc.ceilingAttachmentPoint = fixedAnchor;
    rc.platformAttachmentPoint = dynamicAnchor;
    rc.originalLength = length;
    rc.currentLength = length;
    rc.hasFoundCeiling = true;
    rc.lastWorldRotationDeg = worldRotDeg;

    // Transform
    const float angleRad = atan2(delta.x, -delta.y);
    const float angleDeg = angleRad * (180.0f / 3.14159265f);
    const float absCos = std::abs(std::cos(angleRad));
    const float absSin = std::abs(std::sin(angleRad));
    const float selectionHalfHeight = length * 0.5f;

    Transform ropeTf;
    ropeTf.position = (fixedAnchor + dynamicAnchor) * 0.5f;
    ropeTf.rotation.angle = 0.0f;
    ropeTf.rotation.textureAngle = 0.0f;
    entityMgr->AddComponent(rope, ropeTf);

    // Texture component
    TextureComponent texComp;
    texComp.textureKey = rc.textureKey;
    texComp.HalfSize = Vector2D(
        rc.selectionHalfWidth * absCos + selectionHalfHeight * absSin,
        rc.selectionHalfWidth * absSin + selectionHalfHeight * absCos
    );
    texComp.flipHorizontal = false;
    texComp.alpha = 0.0f;
    entityMgr->AddComponent(rope, texComp);

    // Rotation flag
    RotationFlag ropeRotFlag;
    ropeRotFlag.isRotating = false;
    ropeRotFlag.currentRotation = 0.0f;
    ropeRotFlag.targetRotation = 0.0f;
    entityMgr->AddComponent(rope, ropeRotFlag);

    Entity renderRope = entityMgr->CreateEntity();
    ApplyRopeProperties(renderRope, ropeName + "_Render");
    entityMgr->AddComponent(renderRope, LoadedTag{});

    const float renderOverlap = useCeilingAnchor ? kRopeCeilingVisualOverlap : 0.0f;
    const float renderTrim = useCeilingAnchor ? 0.0f : kRopeWallVisualTrim;
    const float renderLength = std::max(0.0f, length + renderOverlap - renderTrim);
    const Vector2D renderOffset = GetCeilingRenderOffset(delta, renderOverlap - renderTrim);
    const float rawColliderGap = useCeilingAnchor
        ? rawLength
        : std::max(0.0f, GetPlatformColliderLeftEdge(*entityMgr, platformEntity_, platformTf, dynamicAnchor.x) - fixedAnchor.x);
    const bool hideRenderRope = useCeilingAnchor
        ? rawColliderGap <= kHiddenRopeLengthThreshold
        : (rawColliderGap <= kHiddenRopeLengthThreshold && rawLength <= kHiddenRopeLengthThreshold);

    Transform renderTf = ropeTf;
    renderTf.position += renderOffset;
    renderTf.rotation.angle = angleDeg;
    renderTf.rotation.textureAngle = 0.0f;
    entityMgr->AddComponent(renderRope, renderTf);

    TextureComponent renderTexComp;
    renderTexComp.textureKey = hideRenderRope ? "" : rc.textureKey;
    renderTexComp.HalfSize = Vector2D(rc.ropeWidth * 0.5f, renderLength * 0.5f);
    renderTexComp.flipHorizontal = false;
    renderTexComp.alpha = hideRenderRope ? 0.0f : 1.0f;
    entityMgr->AddComponent(renderRope, renderTexComp);

    RotationFlag renderRopeRotFlag = ropeRotFlag;
    renderRopeRotFlag.currentRotation = angleDeg;
    entityMgr->AddComponent(renderRope, renderRopeRotFlag);

    rc.renderEntity = renderRope;
    ropeRuntimeData_[rope] = rc;

    // Store rope entity
    ropeEntities_.push_back(rope);

    Logger::Get().Info(DebugType::Unknown)
        << "Rope created successfully - Position: (" << ropeTf.position.x
        << ", " << ropeTf.position.y << "), Length: " << length << std::endl;
}

void RopeScript::UpdateRopeTransform(Entity rope) const {
    EntityManager* entityMgr = GetEntityManager();
    if (!entityMgr) return;

    if (!entityMgr->HasComponent(rope, TransformType) ||
        !entityMgr->HasComponent(rope, TextureComponentType)) {
        return;
    }

    auto itRc = ropeRuntimeData_.find(rope);
    if (itRc == ropeRuntimeData_.end()) {
        return;
    }
    RopeRuntimeData& rc = itRc->second;
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
    const bool useSpecialPlatformAttachment = UsesCeilingTopRope(*entityMgr);
    const bool useCeilingAnchor = FallsVertically();

    float rotDeltaDeg = worldRotDeg - rc.lastWorldRotationDeg;
    while (rotDeltaDeg > 180.0f) rotDeltaDeg -= 360.0f;
    while (rotDeltaDeg < -180.0f) rotDeltaDeg += 360.0f;
    if (std::abs(rotDeltaDeg) > 0.001f) {
        const float rotDeltaRad = rotDeltaDeg * (3.14159265f / 180.0f);
        const float c = std::cos(rotDeltaRad);
        const float s = std::sin(rotDeltaRad);
        const Vector2D oldAnchor = rc.ceilingAttachmentPoint;
        rc.ceilingAttachmentPoint.x = oldAnchor.x * c - oldAnchor.y * s;
        rc.ceilingAttachmentPoint.y = oldAnchor.x * s + oldAnchor.y * c;
    }
    rc.lastWorldRotationDeg = worldRotDeg;

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
        if (useCeilingAnchor) {
            Vector2D attachPoint = useSpecialPlatformAttachment ? CalculateTopColliderAttachment(*entityMgr, rc.ropeIndex)
                : Vector2D(platformTf.position.x + ((rc.ropeIndex == 0) ? -halfWidth : halfWidth), platformTf.position.y + halfHeight);
            float attachX = attachPoint.x;
            float platformTopY = attachPoint.y;
            float fallbackCeilY = platformTopY + ROPE_OVERHEAD;
            rc.ceilingAttachmentPoint.x = attachX;
            rc.ceilingAttachmentPoint.y = FindCeilingAnchorY(*entityMgr, attachX, platformTopY, fallbackCeilY);
        }
        else {
            Vector2D attachPoint = useSpecialPlatformAttachment ? CalculateTopColliderAttachment(*entityMgr, rc.ropeIndex)
                : Vector2D(platformTf.position.x - halfWidth, platformTf.position.y);
            float attachX = attachPoint.x;
            float attachY = attachPoint.y;
            float fallbackWallX = attachX - HORIZONTAL_FALLBACK_LENGTH;
            rc.ceilingAttachmentPoint.x = FindWallAnchorX(*entityMgr, attachY, attachX, fallbackWallX);
            rc.ceilingAttachmentPoint.y = attachY;
        }

        rc.hasFoundCeiling = true;
    }

    // Calculate attachment point in local space
    Vector2D localEdge = Vector2D::Zero();
    if (!useSpecialPlatformAttachment) {
        localEdge.x = -halfWidth;
        localEdge.y = 0.0f;
    }

    // Transform to world space
    const Vector2D worldEdge(localEdge.x * cosR - localEdge.y * sinR, localEdge.x * sinR + localEdge.y * cosR);
    rc.platformAttachmentPoint = useSpecialPlatformAttachment ? CalculateTopColliderAttachment(*entityMgr, rc.ropeIndex)
        : platformTf.position + worldEdge;

    // Update rope length
    const Vector2D& worldCeiling = rc.ceilingAttachmentPoint;
    const Vector2D delta = rc.platformAttachmentPoint - worldCeiling;
    const float rawCurrentLength = delta.Magnitude();
    rc.currentLength = rawCurrentLength;

    if (rc.currentLength < MIN_ROPE_LENGTH) {
        rc.currentLength = MIN_ROPE_LENGTH;
    }

    // Update rope transform
    const float angleRad = std::atan2(delta.x, -delta.y);
    const float angleDeg = angleRad * (180.0f / 3.14159265f);
    const float absCos = std::abs(std::cos(angleRad));
    const float absSin = std::abs(std::sin(angleRad));
    const float selectionHalfHeight = rc.currentLength * 0.5f;

    ropeTf.position = (worldCeiling + rc.platformAttachmentPoint) * 0.5f;
    ropeTf.rotation.textureAngle = 0.0f;
    ropeTf.rotation.angle = 0.0f;

    if (entityMgr->HasComponent(rope, RotationFlagType)) {
        entityMgr->GetComponent<RotationFlag>(rope).currentRotation = 0.0f;
    }

    // Update texture size
    texComp.textureKey = rc.textureKey;
    texComp.HalfSize.x = rc.selectionHalfWidth * absCos + selectionHalfHeight * absSin;
    texComp.HalfSize.y = rc.selectionHalfWidth * absSin + selectionHalfHeight * absCos;
    texComp.alpha = 0.0f;
    ropeTf.scale = Vector2D(1.0f, 1.0f);

    if (rc.renderEntity != 0 &&
        entityMgr->ValidateEntity(rc.renderEntity) &&
        entityMgr->HasComponent(rc.renderEntity, TransformType) &&
        entityMgr->HasComponent(rc.renderEntity, TextureComponentType)) {
        Transform& renderTf = entityMgr->GetComponent<Transform>(rc.renderEntity);
        TextureComponent& renderTexComp = entityMgr->GetComponent<TextureComponent>(rc.renderEntity);

        const float renderOverlap = useCeilingAnchor ? kRopeCeilingVisualOverlap : 0.0f;
        const float renderTrim = useCeilingAnchor ? 0.0f : kRopeWallVisualTrim;
        const float renderLength = std::max(0.0f, rc.currentLength + renderOverlap - renderTrim);
        const Vector2D renderOffset = GetCeilingRenderOffset(delta, renderOverlap - renderTrim);
        const float rawColliderGap = useCeilingAnchor
            ? rawCurrentLength
            : std::max(0.0f, GetPlatformColliderLeftEdge(*entityMgr, platformEntity_, platformTf, rc.platformAttachmentPoint.x) - rc.ceilingAttachmentPoint.x);
        const bool hideRenderRope = useCeilingAnchor
            ? rawColliderGap <= kHiddenRopeLengthThreshold
            : (rawColliderGap <= kHiddenRopeLengthThreshold && rawCurrentLength <= kHiddenRopeLengthThreshold);

        renderTf.position = ropeTf.position + renderOffset;
        renderTf.rotation = ropeTf.rotation;
        renderTf.rotation.textureAngle = 0.0f;
        renderTf.scale = ropeTf.scale;

        renderTexComp.textureKey = hideRenderRope ? "" : rc.textureKey;
        renderTexComp.HalfSize.x = rc.ropeWidth * 0.5f;
        renderTexComp.HalfSize.y = renderLength * 0.5f;
        renderTexComp.flipHorizontal = false;
        renderTexComp.alpha = hideRenderRope ? 0.0f : 1.0f;

        if (entityMgr->HasComponent(rc.renderEntity, RotationFlagType)) {
            entityMgr->GetComponent<RotationFlag>(rc.renderEntity).currentRotation = angleDeg;
        }
    }

    // Update transformation system
    if (GameManager::Exists() && GameManager::GetInstance().GetSystems().transformation_) {
        GameManager::GetInstance().GetSystems().transformation_->UpdateEntityTransform(rope);
        if (rc.renderEntity != 0 && entityMgr->ValidateEntity(rc.renderEntity)) {
            GameManager::GetInstance().GetSystems().transformation_->UpdateEntityTransform(rc.renderEntity);
        }
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
