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
        Texture currentTexture;
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
                    gfx_.drawText(*item.font, item.textLine, item.screenPos, item.scale, item.color);
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
                bool isUIElement = entityMgr.HasComponent(entity, GUITagType); // Check if entity is a UI element
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

                // --- Frustum Culling ---
                if (activeCamera != 0 && cameraSystem_ && !isUIElement && !isScreenSpaceParticle) {
                    if (worldPos.x + halfSize.x < left || worldPos.x - halfSize.x > right ||
                        worldPos.y + halfSize.y < bottom || worldPos.y - halfSize.y > top) {
                        continue;
                    }
                }
                Vector2D finalScale = { 1.0f, 1.0f };
                //float finalRotation = 0.0f;
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

                // --- Determine Render Order using EntityProperties ---
                const EntityProperties& props = entityMgr.GetProperties(entity);
                int renderOrder = props.customRenderOrder;

                // Use predefined render layer if not custom
                if (props.renderLayer != RenderLayer::Custom) {
                    renderOrder = static_cast<int>(props.renderLayer);
                }

                // --- World → Screen Conversion ---
                Vector2D screenPos = worldPos;
                Vector2D screenHalfSize = halfSize;

                // Check if camera exists AND if this is NOT a UI element
                if (activeCamera != 0 && cameraSystem_ && !isUIElement && !isScreenSpaceParticle) {
                    // Convert center position from world to screen
                    screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                    // Get camera for zoom and aspect ratio
                    const Camera& cam = cameraSystem_->GetCamera(activeCamera);

                    // Convert world-space size to screen-space size
                    // The key is to scale by the camera's zoom and aspect ratio
                    float halfWidth = cam.zoom;
                    float halfHeight = cam.zoom;

                    screenHalfSize.x = halfSize.x / halfWidth;
                    screenHalfSize.y = halfSize.y / halfHeight;
                }

                else if (isScreenSpaceParticle) {
                    // Convert 0..1 to -1..1
                    screenPos.x = (worldPos.x * 2.0f) - 1.0f;
                    screenPos.y = (worldPos.y * 2.0f) - 1.0f;

                    // Do NOT apply camera zoom to size. 
                    // This keeps the particle size constant regardless of camera zoom.
                    screenHalfSize = halfSize;
                }
                // Handle UI Elements (GUITag)
                else if (isUIElement) {
                    // Graphics.cpp shrinks X by (Height/Width). 
                    // We must stretch X by (Width/Height) to counteract it, allowing 
                    // a 1.0 half-size to actually reach the edge of the screen.
                    float aspectRatio = (float)gfx_.getScreenWidth() / (float)gfx_.getScreenHeight();
                    screenHalfSize.x *= aspectRatio;
                }

                // Calculate final screen-space top-left and size
                Vector2D topLeft = { screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y };
                Vector2D fullSize = { screenHalfSize.x * 2.0f, screenHalfSize.y * 2.0f };

                // --- Create Batched Sprite ---
                BatchedSprite batchedSprite;
                batchedSprite.topLeft = topLeft;
                batchedSprite.size = fullSize;
                batchedSprite.rotation = rotationRad;

                // --- UV Animation ---
                if (entityMgr.HasComponent(entity, AnimationComponentType)) {
                    const AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
                    batchedSprite.minUV = animComp.minUV;
                    batchedSprite.maxUV = animComp.maxUV;
                }
                else {
                    batchedSprite.minUV = { 0.0f, 0.0f };
                    batchedSprite.maxUV = { 1.0f, 1.0f };
                }

                // Swaps the U coordinates if the flipX flag is set on the texture component
                if (sprite.flipHorizontal) {
                    std::swap(batchedSprite.minUV.x, batchedSprite.maxUV.x);
                }

                // UV Inset to prevent texture bleeding
                /*float uvInset = 0.005f;
                batchedSprite.minUV.x += uvInset;
                batchedSprite.minUV.y += uvInset;
                batchedSprite.maxUV.x -= uvInset;
                batchedSprite.maxUV.y -= uvInset;*/

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
        float zoom = 1.0f, zoomRefSize= 540.0f, zoomRatio = 1.0f;
        Vector2D camPos = { 0.0f, 0.0f };
        Vector2D camSize = { (float)gfx_.getScreenWidth(), (float)gfx_.getScreenHeight() };

        if (activeCamera != 0 && cameraSystem_) {
            const Camera& cam = cameraSystem_->GetCamera(activeCamera);
            zoom = cam.zoom;
            camPos = cam.position;
            camSize = cam.size;
            zoomRatio = zoomRefSize / zoom;
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

            Font& font = rcm_.get<Font>(textComp.fontKey);
            const Vector2D& textPos = entityMgr.GetComponent<Transform>(entity).position;
            bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

            // --- Calculate Screen Position ---
            Vector2D screenTextPos = textPos;

            if (!isUIElement) {
                // Subtract Camera Position from Text Position
                float relativeX = textPos.x - camPos.x;
                float relativeY = textPos.y - camPos.y;

                // offset so that pos is the middle of the text
                relativeX -= textComp.halfSize.x;
                relativeY += textComp.halfSize.y;

                // Convert Relative Position to NDC
                screenTextPos = Vector2D(
                    (relativeX / camSize.x) * 2.0f * zoomRatio,
                    (relativeY / camSize.y) * 2.0f * zoomRatio
                );
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
                screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                // Get camera for zoom and aspect ratio
                const Camera& cam = cameraSystem_->GetCamera(activeCamera);

                // Convert world-space size to screen-space size
                float halfWidth = cam.zoom * cam.aspectRatio;
                float halfHeight = cam.zoom;

                screenHalfSize.x = halfSize.x / halfWidth;
                screenHalfSize.y = halfSize.y / halfHeight;
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


} // namespace Framework