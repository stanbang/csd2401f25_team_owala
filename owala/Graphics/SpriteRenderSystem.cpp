/*********************************************************************//**
 * \file   SpriteRenderSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author 
 *      Ng Jing Wen (10%)
 *      Yar Xuan (10%)
 *
 * \brief
 *      Implements the SpriteRenderSystem responsible for rendering sprites,
 *      text, and optional collider outlines within the ECS framework.
 *
 * \details
 *      The SpriteRenderSystem handles:
 *        - Sprite batching and sorting by render order and texture.
 *        - World-to-screen coordinate transformation using CameraSystem.
 *        - Layered rendering for background, objects, enemies, players, and UI.
 *        - Text rendering with automatic line wrapping and zoom correction.
 *        - Optional collider visualization for debugging purposes.
 *
 *      Rendering flow per frame:
 *        1. Begin frame clearing the color buffer.
 *        2. Render sprites in sorted order.
 *        3. Render text components.
 *        4. Optionally draw collider outlines.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "SpriteRenderSystem.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "WindowsSystem.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    /**
     * \brief Main per-frame update function for sprite rendering.
     *
     * Clears the screen, renders all sprite batches, and optionally
     * draws collider outlines if enabled.
     */
    void SpriteRenderSystem::Update() {
        GameManager::GetInstance().GetSystems().gfx_->beginFrame(clearColor_);
        RenderSprites();
        RenderAttackCircles();
        if (drawColliders_) {
            RenderColliderOutlines();
        }
    }

    /**
     * \brief Renders all visible sprites
     *
     * Collects all entities with active `TextureComponent`, applies
     * world-to-screen transformations, performs frustum culling,
     * sorts by render order and texture, and draws batched sprites.
     */
    void SpriteRenderSystem::RenderSprites() {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;

        // Retrieve active camera
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        std::vector<RenderItem> renderQueue;

        // --- Collect Sprites ---
        CollectSprites(activeCamera, renderQueue);

        // --- Collect Text ---
        CollectText(activeCamera, renderQueue);

        // Sort Everything by RenderOrder
        // We use stable_sort to preserve the order of creation for items with equal keys
        std::stable_sort(renderQueue.begin(), renderQueue.end());

        // Execute Render Loop (Batching Interleaved)
        std::vector<BatchedSprite> currentBatch;
        Texture currentTexture{};
        bool batchStarted = false;

        for (const auto& item : renderQueue) {
            if (item.isText) {
                // -- Text Encountered --
                // If we have sprites waiting in the batch, draw them now to preserve order
                if (batchStarted && !currentBatch.empty()) {
                    gfx_.drawSpriteBatch(currentTexture, currentBatch);
                    currentBatch.clear();
                    batchStarted = false;
                }

                // Draw the text immediately
                if (item.font) {
                    gfx_.drawText(*item.font, item.textLine, item.screenPos, item.scale, item.color, item.rotation);
                }
            }
            else {
                // -- Sprite Encountered --
                // If texture changed, flush the previous batch
                if (batchStarted && (item.texture != currentTexture)) {
                    gfx_.drawSpriteBatch(currentTexture, currentBatch);
                    currentBatch.clear();
                }

                // Add to current batch
                currentTexture = item.texture;
                currentBatch.push_back(item.spriteData);
                batchStarted = true;
            }
        }

        // Flush remaining sprites at the end
        if (batchStarted && !currentBatch.empty()) {
            gfx_.drawSpriteBatch(currentTexture, currentBatch);
        }
    }

    void SpriteRenderSystem::CollectSprites(Entity activeCamera, std::vector<RenderItem>& queue) {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        EntityManager entityMgr(gm.GetECS());
        ResourceManager& resources_ = *gm.GetSystems().resourceManager_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;

        float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;
        if (activeCamera != 0 && cameraSystem_) {
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        }

        // --- Collect Sprite Entities ---
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(true, TextureComponentType, TransformType, LoadedTagType)) {
            TextureComponent& sprite = entityMgr.GetComponent<TextureComponent>(entity);
            if (sprite.textureKey.empty()) continue;

            try {
                const Transform& transform = entityMgr.GetComponent<Transform>(entity);
                const Vector2D& worldPos = transform.position;
                bool isUIElement = entityMgr.HasComponent(entity, GUITagType);
                bool isScreenSpaceParticle = false;

                // Check if particle is screen space
                if (entityMgr.HasComponent(entity, ParticleComponentType)) {
                    const ParticleComponent& pc = entityMgr.GetComponent<ParticleComponent>(entity);
                    isScreenSpaceParticle = pc.isScreenSpace;
                }

                // Determine half size based on available components
                Vector2D halfSize;
                if (entityMgr.HasComponent(entity, TextureComponentType))
                    halfSize = sprite.HalfSize;
                else if (entityMgr.HasComponent(entity, AABBColliderType))
                    halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
                else
                    halfSize = sprite.HalfSize;

                Vector2D finalScale = { 1.0f, 1.0f };
                // --- Apply Scale ---
                const Vector2D& scale = transform.scale;
                finalScale = scale;
                halfSize.x *= finalScale.x;
                halfSize.y *= finalScale.y;

                // --- Rotation (degrees → radians) ---
                float rotationRad = 0.0f;
                bool hasRotationFlag = entityMgr.HasComponent(entity, RotationFlagType);

                if (hasRotationFlag) {
                    const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    const Transform::Rotation& rotation = transform.rotation;
                    float worldRotationRad = rotation.textureAngle;
                    float localRotationRad = static_cast<float>(rotationFlag.currentRotation * (M_PI / 180.0));
                    rotationRad = worldRotationRad + localRotationRad;
                }
                else {
                    const Transform::Rotation& rotation = transform.rotation;
                    rotationRad = rotation.textureAngle;
                }

                // --- Frustum Culling (rotation-aware AABB) ---
                if (activeCamera != 0 && cameraSystem_ && !isUIElement && !isScreenSpaceParticle) {
                    // Project the rotated quad onto the axes to get a tight AABB for culling.
                    // For a rectangle rotated by θ: AABB half-extents = |hx·cosθ| + |hy·sinθ| , |hx·sinθ| + |hy·cosθ|
                    float cosA = std::abs(std::cos(rotationRad));
                    float sinA = std::abs(std::sin(rotationRad));
                    float cullHalfX = halfSize.x * cosA + halfSize.y * sinA;
                    float cullHalfY = halfSize.x * sinA + halfSize.y * cosA;
                    if (worldPos.x + cullHalfX < left || worldPos.x - cullHalfX > right ||
                        worldPos.y + cullHalfY < bottom || worldPos.y - cullHalfY > top) {
                        continue;
                    }
                }

                // --- Determine Render Order using EntityProperties ---
                const EntityProperties& props = entityMgr.GetProperties(entity);
                int renderOrder = props.customRenderOrder;

                if (props.renderLayer != RenderLayer::Custom) {
                    renderOrder = static_cast<int>(props.renderLayer);
                }

                // --- World → Screen Conversion ---
                Vector2D screenPos = worldPos;
                Vector2D screenHalfSize = halfSize;

                if (activeCamera != 0 && cameraSystem_ && !isUIElement && !isScreenSpaceParticle) {
                    screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                    const Camera& cam = cameraSystem_->GetCamera(activeCamera);

                    // Convert world-space size to screen-space size
                    // The key is to scale by the camera's zoom and aspect ratio
                    screenHalfSize.x = halfSize.x / cam.zoom;
                    screenHalfSize.y = halfSize.y / cam.zoom;
                }
                else if (isScreenSpaceParticle) {
                    screenPos.x = (worldPos.x * 2.0f) - 1.0f;
                    screenPos.y = (worldPos.y * 2.0f) - 1.0f;
                    screenHalfSize = halfSize;
                }
                else if (isUIElement) {
                    float aspectRatio = (float)gfx_.getScreenWidth() / (float)gfx_.getScreenHeight();
                    screenHalfSize.x *= aspectRatio;
                }

                // Calculate final screen-space top-left and size
                Vector2D topLeft = { screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y };
                Vector2D fullSize = { screenHalfSize.x * 2.0f, screenHalfSize.y * 2.0f };

                // --- Create Batched Sprite ---
                BatchedSprite batchedSprite;
                batchedSprite.topLeft = topLeft;
                batchedSprite.rotation = rotationRad;
                batchedSprite.color = { 1.0f, 1.0f, 1.0f, sprite.alpha }; // READ ALPHA FROM COMPONENT

                // --- UV Selection ---
                if (sprite.overrideUV) {
                    // maxUV.x stores the fill fraction (0–1), set by GuiSystem.
                    float fractionX = sprite.maxUV.x - sprite.minUV.x;
                    float fractionY = sprite.maxUV.y - sprite.minUV.y;

                    batchedSprite.minUV = sprite.minUV;
                    batchedSprite.maxUV = sprite.maxUV;

                    // Calculate the squashed size
                    batchedSprite.size = {
                        screenHalfSize.x * 2.0f * fractionX,
                        screenHalfSize.y * 2.0f * fractionY
                    };

                    // Grab the same aspect ratio correction used in Graphics.cpp
                    float aspectCorrection = (float)gfx_.getScreenHeight() / (float)gfx_.getScreenWidth();

                    // 1. Calculate the TRUE screen-space left edge (accounting for aspect correction)
                    float trueLeftEdgeX = screenPos.x - (screenHalfSize.x * aspectCorrection);

                    // 2. Calculate what the center (cx) needs to be in Graphics.cpp for this to stay anchored
                    float targetCx = trueLeftEdgeX + (batchedSprite.size.x * 0.5f * aspectCorrection);

                    // 3. Reverse-engineer topLeft.x so that `cx = topLeft.x + size.x * 0.5f` in Graphics.cpp works perfectly
                    batchedSprite.topLeft.x = targetCx - (batchedSprite.size.x * 0.5f);

                    // Y axis is NOT affected by aspect correction in Graphics.cpp, so it is just the top edge
                    batchedSprite.topLeft.y = screenPos.y - screenHalfSize.y;
                }
				// If no UV override, use full texture and original size
                else {
                    batchedSprite.topLeft = topLeft;
                    batchedSprite.size = fullSize;

                    if (entityMgr.HasComponent(entity, AnimationComponentType)) {
                        const AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
                        batchedSprite.minUV = animComp.minUV;
                        batchedSprite.maxUV = animComp.maxUV;
                    }
                    else {
                        batchedSprite.minUV = { 0.0f, 0.0f };
                        batchedSprite.maxUV = { 1.0f, 1.0f };
                    }
                }

                // Swaps the U coordinates if the flipX flag is set on the texture component
                if (sprite.flipHorizontal) {
                    std::swap(batchedSprite.minUV.x, batchedSprite.maxUV.x);
                }

                // --- Half-texel UV inset to prevent texture bleeding at tile edges ---
                // The inset is scaled proportionally to the UV range of this sprite so that
                // a multi-tile sprite (large UV range) gets the same visual inset as a
                // single-tile sprite (small UV range), keeping all tiles consistent.
                // SKIP FOR OVERRIDE UV
                if (!sprite.overrideUV) {
                    GLint texW = 1, texH = 1;
                    Texture texID = resources_.get<Texture>(sprite.textureKey);
                    glBindTexture(GL_TEXTURE_2D, texID);
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW);
                    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);
                    glBindTexture(GL_TEXTURE_2D, 0);

                    // UV range this sprite occupies in the texture (1.0 for a full-texture tile)
                    float uvRangeX = batchedSprite.maxUV.x - batchedSprite.minUV.x;
                    float uvRangeY = batchedSprite.maxUV.y - batchedSprite.minUV.y;

                    // Half-texel in UV space, scaled so multi-tile sprites get a proportionally
                    // equivalent inset to a single-tile sprite occupying the full texture.
                    float uInset = (0.5f / static_cast<float>(texW)) * uvRangeX;
                    float vInset = (0.5f / static_cast<float>(texH)) * uvRangeY;

                    batchedSprite.minUV.x += uInset;
                    batchedSprite.minUV.y += vInset;
                    batchedSprite.maxUV.x -= uInset;
                    batchedSprite.maxUV.y -= vInset;
                }
                RenderItem item;
                item.isText = false;
                item.renderOrder = renderOrder;
                item.texture = resources_.get<Texture>(sprite.textureKey);
                item.spriteData = batchedSprite;

                queue.push_back(item);
            }
            catch (const std::exception& e) {
                Logger::Get().Error(DebugType::SpriteRenderSystem)
                    << "Failed to render entity " << entity
                    << " (Bad key: " << sprite.textureKey << "): " << e.what() << std::endl;
                continue;
            }
        }
    }

    void SpriteRenderSystem::CollectText(Entity activeCamera, std::vector<RenderItem>& queue) {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        EntityManager entityMgr(gm.GetECS());
        ResourceManager& rcm_ = *gm.GetSystems().resourceManager_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;

        // Retrieve active camera data
        float zoom = 1.0f, zoomRefSize = 350.0f, zoomRatio = 1.0f;
        Vector2D camPos = { 0.0f, 0.0f };
        Vector2D camSize = { (float)gfx_.getScreenWidth(), (float)gfx_.getScreenHeight() };

        if (activeCamera != 0 && cameraSystem_) {
            try {
                const Camera& cam = cameraSystem_->GetCamera(activeCamera);
                zoom = cam.zoom;
                camPos = cam.position;
                camSize = cam.size;
                zoomRatio = zoomRefSize / zoom;
            }
            catch (const std::exception& e) {
                Logger::Get().Error(DebugType::SpriteRenderSystem)
                    << "CollectText: failed to get camera " << activeCamera
                    << ": " << e.what() << std::endl;
            }
        }

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(true, TextComponentType, TransformType, LoadedTagType)) {
            const auto& textComp = entityMgr.GetComponent<TextComponent>(entity);
            if (textComp.fontKey.empty() || textComp.textToDraw.empty()) continue;

            // Determine Render Order
            const EntityProperties& props = entityMgr.GetProperties(entity);
            int renderOrder = props.customRenderOrder;
            if (props.renderLayer != RenderLayer::Custom) {
                renderOrder = static_cast<int>(props.renderLayer);
            }

            Font* fontPtr = nullptr;
            try {
                fontPtr = &rcm_.get<Font>(textComp.fontKey);
            }
            catch (const std::runtime_error&) {
                Logger::Get().Error(DebugType::SpriteRenderSystem) << "Missing font '" << textComp.fontKey << "' — skipping text entity " << entity << std::endl;
                continue;
            }
            Font& font = *fontPtr;

            const Transform& transform = entityMgr.GetComponent<Transform>(entity);
            const Vector2D& textPos = transform.position;
            bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

            // --- Rotation ---
            float rotationRad = 0.0f;
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                float worldRotationRad = transform.rotation.textureAngle;
                float localRotationRad = static_cast<float>(rotationFlag.currentRotation * (M_PI / 180.0));
                rotationRad = worldRotationRad + localRotationRad;
            }
            else {
                rotationRad = transform.rotation.textureAngle;
            }

            // --- Calculate Screen Position ---
            Vector2D windowSize = gm.GetSystems().win_->getWindowSize();
            Vector2D screenTextPos = textPos;

            if (!isUIElement) {
                // Anchor to World Position
                // Use the standard camera function so text stays attached to the Gizmo/Entity
                if (activeCamera != 0 && cameraSystem_) {
                    screenTextPos = cameraSystem_->WorldToScreen(textPos, activeCamera);
                }

                // Apply Offset in Screen Space (NDC)
                // Convert halfSize (Pixels) -> NDC. 
                // We only scale by 'zoomRatio' because textComp.halfSize already includes textComp.scale
                float ndcHalfW = (textComp.halfSize.x * zoomRatio / windowSize.x) * 2.0f;
                float ndcHalfH = (textComp.halfSize.y * zoomRatio / windowSize.y) * 2.0f;

                screenTextPos.x -= ndcHalfW;
                screenTextPos.y += ndcHalfH;
            }
            else {
                // Texts with GUI tag already use screen coords
                // offset so that pos is the middle of the text
                screenTextPos.x -= textComp.halfSize.x;
                screenTextPos.y += textComp.halfSize.y;
            }

            // get line height for offsetting the y when rendering multiple lines of text
            float line_height_px = font.GetLineHeight(textComp.scale);
            float line_height_ndc = (line_height_px / (float)gfx_.getScreenHeight()) * 2.0f;

            // Create a RenderItem for each line of text
            Vector2D currentLinePos = screenTextPos;
            for (const std::string& line : textComp.wrappedLines) {
                RenderItem item;
                item.isText = true;
                item.renderOrder = renderOrder;
                item.font = &font;
                item.textLine = line;
                item.screenPos = currentLinePos;
                item.scale = textComp.scale * (isUIElement ? 1.0f : zoomRatio);
                item.color = textComp.color;
                item.rotation = rotationRad;

                queue.push_back(item);

                // Move cursor down for next line
                currentLinePos.y -= line_height_ndc * (isUIElement ? 1.0f : zoomRatio);
            }
        }
    }

    /**
     * \brief Renders collider outlines for debugging.
     *
     * Draws green outlines for all active AABB colliders
     * in the currently active scene. Uses camera transformations
     * to maintain visual alignment with world coordinates.
     */
    void SpriteRenderSystem::RenderColliderOutlines() {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;
        EntityManager entityMgr(gm.GetECS());

        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        const std::string activeSceneName = SceneManager::GetInstance().GetActiveScene().name;
        const std::vector<Entity> platformEntities = entityMgr.GetAllEntitiesWithComponent(true, PlatformTagType);

        auto tryGetWorldAABB = [&](Entity entity, AABB_Bounds& outBounds, Vector2D& outColliderCenter, Vector2D& outHalfSize) -> bool {
            if (!entityMgr.HasComponent(entity, TransformType) || !entityMgr.HasComponent(entity, AABBColliderType)) {
                return false;
            }

            const Transform& transform = entityMgr.GetComponent<Transform>(entity);
            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
            outHalfSize = collider.HalfSize;

            float deg = 0.0f;
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                deg = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
            }
            deg += transform.rotation.colliderAngle * (180.0f / static_cast<float>(M_PI));

            Vector2D rotatedOffset = collider.Offset;
            if (std::abs(deg) > 0.01f) {
                float rotRad = deg * (static_cast<float>(M_PI) / 180.0f);
                Matrix3x3 rotMat = Matrix3x3::CreateRotation(rotRad);
                rotatedOffset = rotMat * collider.Offset;
            }

            outColliderCenter = transform.position + rotatedOffset;
            outBounds = PhysicsSystem::GetRotatedAABB(outColliderCenter, outHalfSize, deg);
            return true;
        };

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, AABBColliderType)) {
            if (!entityMgr.HasComponent(e, SceneOwnedType)) continue;
            const SceneOwned& sceneOwned = entityMgr.GetComponent<SceneOwned>(e);
            if (sceneOwned.sceneName != activeSceneName) continue;

            AABB_Bounds rotatedBounds{};
            Vector2D colliderWorldPos = Vector2D::Zero();
            Vector2D halfSize = Vector2D::Zero();
            if (!tryGetWorldAABB(e, rotatedBounds, colliderWorldPos, halfSize)) continue;

            bool isBirdEnemy = false;
            if (entityMgr.HasComponent(e, EnemyTagType)) {
                const std::string& entityName = entityMgr.GetProperties(e).name;
                isBirdEnemy = (entityName.find("bird") != std::string::npos || entityName.find("Bird") != std::string::npos);

                if (!isBirdEnemy && entityMgr.HasComponent(e, FSMComponentType)) {
                    const std::string& fsmPath = entityMgr.GetComponent<FSMComponent>(e).fsmDefinitionPath;
                    isBirdEnemy = (fsmPath.find("bird") != std::string::npos || fsmPath.find("Bird") != std::string::npos);
                }
            }

            bool touchingPlatform = false;
            Vector2D contactNormal = Vector2D::Zero();
            float bestPenetration = 0.0f;

            if (isBirdEnemy) {
                for (const Entity platform : platformEntities) {
                    if (platform == e) continue;
                    if (!entityMgr.HasComponent(platform, SceneOwnedType)) continue;
                    if (entityMgr.GetComponent<SceneOwned>(platform).sceneName != activeSceneName) continue;

                    AABB_Bounds platformBounds{};
                    Vector2D platformCenter = Vector2D::Zero();
                    Vector2D platformHalfSize = Vector2D::Zero();
                    if (!tryGetWorldAABB(platform, platformBounds, platformCenter, platformHalfSize)) {
                        continue;
                    }

                    const bool overlapX = (rotatedBounds.max.x >= platformBounds.min.x) && (platformBounds.max.x >= rotatedBounds.min.x);
                    const bool overlapY = (rotatedBounds.max.y >= platformBounds.min.y) && (platformBounds.max.y >= rotatedBounds.min.y);
                    if (!overlapX || !overlapY) continue;

                    const float overlapLeft = rotatedBounds.max.x - platformBounds.min.x;
                    const float overlapRight = platformBounds.max.x - rotatedBounds.min.x;
                    const float overlapTop = rotatedBounds.max.y - platformBounds.min.y;
                    const float overlapBottom = platformBounds.max.y - rotatedBounds.min.y;

                    const float minOverlapX = std::min(overlapLeft, overlapRight);
                    const float minOverlapY = std::min(overlapTop, overlapBottom);

                    Vector2D candidateNormal = Vector2D::Zero();
                    float penetration = 0.0f;
                    if (minOverlapX < minOverlapY) {
                        candidateNormal = (overlapLeft < overlapRight) ? Vector2D::Left() : Vector2D::Right();
                        penetration = minOverlapX;
                    }
                    else {
                        candidateNormal = (overlapTop < overlapBottom) ? Vector2D::Down() : Vector2D::Up();
                        penetration = minOverlapY;
                    }

                    if (!touchingPlatform || penetration < bestPenetration) {
                        touchingPlatform = true;
                        bestPenetration = penetration;
                        contactNormal = candidateNormal;
                    }
                }
            }

            Color outlineColor = { 0.0f, 1.0f, 0.0f, 1.0f };
            if (touchingPlatform) {
                outlineColor = { 1.0f, 0.0f, 0.0f, 1.0f };
            }

            // Convert world bounds to screen space
            Vector2D screenMin = rotatedBounds.min;
            Vector2D screenMax = rotatedBounds.max;

            if (activeCamera != 0 && cameraSystem_) {
                screenMin = cameraSystem_->WorldToScreen(rotatedBounds.min, activeCamera);
                screenMax = cameraSystem_->WorldToScreen(rotatedBounds.max, activeCamera);
            }

            // Draw the axis-aligned bounding box
            Vector2D corners[4] = {
                {screenMin.x, screenMin.y},  // bottom-left
                {screenMax.x, screenMin.y},  // bottom-right
                {screenMax.x, screenMax.y},  // top-right
                {screenMin.x, screenMax.y}   // top-left
            };

            for (int i = 0; i < 4; ++i) {
                int next = (i + 1) % 4;
                gfx_.drawLine(corners[i], corners[next], outlineColor);
            }

            if (touchingPlatform) {
                const float arrowLenWorld = std::max(25.0f, std::max(halfSize.x, halfSize.y));
                Vector2D arrowStart = colliderWorldPos;
                Vector2D arrowEnd = colliderWorldPos + contactNormal * arrowLenWorld;

                if (activeCamera != 0 && cameraSystem_) {
                    arrowStart = cameraSystem_->WorldToScreen(arrowStart, activeCamera);
                    arrowEnd = cameraSystem_->WorldToScreen(arrowEnd, activeCamera);
                }

                const Color arrowColor = { 1.0f, 0.9f, 0.2f, 1.0f };
                gfx_.drawLine(arrowStart, arrowEnd, arrowColor);

                Vector2D dir = arrowEnd - arrowStart;
                const float mag = dir.Magnitude();
                if (mag > 0.0001f) {
                    dir /= mag;
                    Vector2D perp = { -dir.y, dir.x };
                    const float headSize = (activeCamera != 0 && cameraSystem_) ? 0.02f : std::max(4.0f, arrowLenWorld * 0.25f);
                    const Vector2D headBase = arrowEnd - dir * (headSize * 1.5f);
                    const Vector2D headLeft = headBase + perp * headSize;
                    const Vector2D headRight = headBase - perp * headSize;
                    gfx_.drawLine(arrowEnd, headLeft, arrowColor);
                    gfx_.drawLine(arrowEnd, headRight, arrowColor);
                }
            }
        }
    }

    void SpriteRenderSystem::RenderAttackCircles() {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;
        EntityManager entityMgr(gm.GetECS());

        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, FSMComponentType)) {
            FSMComponent& fsm = entityMgr.GetComponent<FSMComponent>(e);
            if (fsm.attackCircleRadius <= 0.0f) continue;
            if (!entityMgr.HasComponent(e, TransformType)) continue;
            if (!entityMgr.HasComponent(e, SceneOwnedType)) continue;

            const SceneOwned& sceneOwned = entityMgr.GetComponent<SceneOwned>(e);
            if (sceneOwned.sceneName != SceneManager::GetInstance().GetActiveScene().name) continue;

            const Transform& transform = entityMgr.GetComponent<Transform>(e);
            Vector2D worldPos = transform.position;
            float worldRadius = fsm.attackCircleRadius;

            // Use textured sprite if a texture key is set
            if (!fsm.attackCircleTextureKey.empty()) {
                try {
                    Texture& tex = gm.GetSystems().resourceManager_->get<Texture>(fsm.attackCircleTextureKey);

                    // Convert center and edge point to NDC to get screen-space radius
                    Vector2D screenCenter = worldPos;
                    Vector2D screenEdge = { worldPos.x + worldRadius, worldPos.y };
                    if (activeCamera != 0 && cameraSystem_) {
                        screenCenter = cameraSystem_->WorldToScreen(worldPos, activeCamera);
                        screenEdge = cameraSystem_->WorldToScreen(screenEdge, activeCamera);
                    }
                    float screenRadius = std::abs(screenEdge.x - screenCenter.x);
                    // compensate for smaller VFX frame asset (old 2048 -> new 818)
                    constexpr float kWitchCircleVisualScale = 2048.0f / 1024.0f; // ~2.5037
                    Vector2D halfSize = { screenRadius * kWitchCircleVisualScale,
                                          screenRadius * kWitchCircleVisualScale };

                    // Handle animated attack circle VFX
                    if (fsm.attackCircleAnimated && !fsm.attackCircleAnimTextures.empty()) {
                        if (fsm.attackCircleAnimFrameCounts.empty() ||
                            fsm.attackCircleAnimFrameCounts.size() != fsm.attackCircleAnimTextures.size()) {
                            Logger::Get().Error(DebugType::SpriteRenderSystem)
                                << "Invalid witch attack VFX data: frame-count metadata mismatch, falling back to static draw." << std::endl;
                            gfx_.drawSprite(tex, screenCenter, halfSize);
                            continue;
                        }

                        // Calculate which spritesheet and which sub-frame within that sheet
                        int sheetIndex = 0;
                        int frameAccumulator = 0;
                        int subFrame = 0;
                        
                        for (size_t i = 0; i < fsm.attackCircleAnimFrameCounts.size(); ++i) {
                            if (fsm.attackCircleAnimFrame < frameAccumulator + fsm.attackCircleAnimFrameCounts[i]) {
                                sheetIndex = static_cast<int>(i);
                                subFrame = fsm.attackCircleAnimFrame - frameAccumulator;
                                break;
                            }
                            frameAccumulator += fsm.attackCircleAnimFrameCounts[i];
                        }
                        
                        // Get the frame count for the current spritesheet
                        int framesInCurrentSheet = (sheetIndex < static_cast<int>(fsm.attackCircleAnimFrameCounts.size())) 
                            ? fsm.attackCircleAnimFrameCounts[sheetIndex] 
                            : 1;
                        
                        // Calculate UV coordinates for the current sub-frame
                        // Frames are laid out horizontally in each spritesheet
                        if (framesInCurrentSheet <= 0) {
                            Logger::Get().Error(DebugType::SpriteRenderSystem)
                                << "Invalid witch attack VFX data: frame count <= 0, falling back to static draw." << std::endl;
                            gfx_.drawSprite(tex, screenCenter, halfSize);
                            continue;
                        }

                        float uvWidth = 1.0f / framesInCurrentSheet;
                        float uvHeight = 1.0f;
                        int col = subFrame;
                        int row = 0;
                        
                        Vector2D minUV = { col * uvWidth, row * uvHeight };
                        Vector2D maxUV = { minUV.x + uvWidth, minUV.y + uvHeight };
                        
                        gfx_.drawSprite(tex, screenCenter, halfSize, 0.0f, minUV, maxUV);
                    }
                    else {
                        gfx_.drawSprite(tex, screenCenter, halfSize);
                    }
                }
                catch (...) {
                    // Fallback: texture not found, skip rendering
                }
                continue;
            }

            // Fallback: draw red circle using line segments
            const int segments = 48;
            const float twoPi = 2.0f * static_cast<float>(M_PI);

            float intensity = worldRadius / 200.0f;
            if (intensity > 1.0f) intensity = 1.0f;
            Color circleColor = { 1.0f, 0.15f, 0.1f, 0.5f + 0.4f * intensity };

            for (int i = 0; i < segments; ++i) {
                float angle1 = static_cast<float>(i) / segments * twoPi;
                float angle2 = static_cast<float>(i + 1) / segments * twoPi;

                Vector2D p1 = { worldPos.x + std::cos(angle1) * worldRadius,
                                worldPos.y + std::sin(angle1) * worldRadius };
                Vector2D p2 = { worldPos.x + std::cos(angle2) * worldRadius,
                                worldPos.y + std::sin(angle2) * worldRadius };

                if (activeCamera != 0 && cameraSystem_) {
                    p1 = cameraSystem_->WorldToScreen(p1, activeCamera);
                    p2 = cameraSystem_->WorldToScreen(p2, activeCamera);
                }

                gfx_.drawLine(p1, p2, circleColor);
            }
        }
    }


} // namespace Framework
