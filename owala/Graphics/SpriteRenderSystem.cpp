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

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(true, AABBColliderType)) {
            if (!entityMgr.HasComponent(e, SceneOwnedType)) continue;
            const SceneOwned& sceneOwned = entityMgr.GetComponent<SceneOwned>(e);
            if (sceneOwned.sceneName != SceneManager::GetInstance().GetActiveScene().name) continue;

            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(e);
            if (!entityMgr.HasComponent(e, TransformType)) continue;

            const Transform& transform = entityMgr.GetComponent<Transform>(e);
            const Vector2D& worldPos = transform.position;
            const Vector2D& halfSize = collider.HalfSize;
            
            // Get rotation if entity has it
            float deg = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
               deg = entityMgr.GetComponent<RotationFlag>(e).currentRotation;
            }

            deg += transform.rotation.colliderAngle * (180.0f / 3.14159265359f);

            Vector2D rotatedOffset = collider.Offset;
            if (std::abs(deg) > 0.01f) {
                float rotRad = deg * (static_cast<float>(M_PI) / 180.0f);
                Matrix3x3 rotMat = Matrix3x3::CreateRotation(rotRad);
                rotatedOffset = rotMat * collider.Offset;
            }

            Vector2D colliderWorldPos = worldPos + rotatedOffset;

            Vector2D screenPos = worldPos;
            Vector2D screenHalfSize = halfSize;

            if (activeCamera != 0 && cameraSystem_) {
                try {
                    screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                    // Get camera for zoom and aspect ratio
                    const Camera& cam = cameraSystem_->GetCamera(activeCamera);

                    // Convert world-space size to screen-space size
                    float halfWidth = cam.zoom * cam.aspectRatio;
                    float halfHeight = cam.zoom;

                    screenHalfSize.x = halfSize.x / halfWidth;
                    screenHalfSize.y = halfSize.y / halfHeight;
                }
                catch (const std::exception& e) {
                    Logger::Get().Error(DebugType::SpriteRenderSystem)
                        << "RenderColliderOutlines: failed to get camera " << activeCamera
                        << ": " << e.what() << std::endl;
                    continue;
                }
            }

            Vector2D scaledHalfSize = halfSize;

            // Apply Scale component if present
            const Vector2D& scale = transform.scale;
            scaledHalfSize.x *= scale.x;
            scaledHalfSize.y *= scale.y;
           
            // Use the rotated AABB calculation
            AABB_Bounds rotatedBounds = PhysicsSystem::GetRotatedAABB(colliderWorldPos, halfSize, deg);

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
                gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f });
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
                    Vector2D halfSize = { screenRadius, screenRadius };

                    // Handle animated attack circle VFX
                    if (fsm.attackCircleAnimated && !fsm.attackCircleAnimTextures.empty()) {
                        const int framesPerSheet = 2;
                        int subFrame = fsm.attackCircleAnimFrame % framesPerSheet;
                        
                        // Calculate UV coordinates for the current sub-frame.
                        // Witch attack circle VFX sheets are 2-frame horizontal strips.
                        float uvWidth = 0.5f;
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
