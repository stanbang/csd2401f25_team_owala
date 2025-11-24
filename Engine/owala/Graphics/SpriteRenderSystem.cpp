/*********************************************************************//**
 * \file   SpriteRenderSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
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

#include "SpriteRenderSystem.h"
#include "EntityManager.h"
#include "SceneManager.h"
#include "Matrix3x3.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <tuple>

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
        gfx_.beginFrame(clearColor_);
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
        EntityManager entityMgr(ecs_);

        // Retrieve active camera
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;
        if (activeCamera != 0 && cameraSystem_) {
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        }

        // --- Sprite Batching ---
        struct BatchKey {
            Texture texture;
            int renderOrder;
            bool operator<(const BatchKey& other) const {
                return std::tie(renderOrder, texture) < std::tie(other.renderOrder, other.texture);
            }
        };

        std::map<BatchKey, std::vector<BatchedSprite>> batches;

        // --- Collect Sprite Entities ---
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextureComponentType, PositionType, ActiveTagType)) {
            TextureComponent& sprite = entityMgr.GetComponent<TextureComponent>(entity);
            if (sprite.textureKey.empty()) continue;

            try {
                const Vector2D& worldPos = entityMgr.GetComponent<Position>(entity).Pos;
                bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

                // Determine half size based on available components
                Vector2D halfSize;
                if (entityMgr.HasComponent(entity, TextureComponentType))
                    halfSize = sprite.HalfSize;
                else if (entityMgr.HasComponent(entity, AABBColliderType))
                    halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
                else
                    halfSize = sprite.HalfSize;

                // --- Frustum Culling ---
                if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
                    if (worldPos.x + halfSize.x < left || worldPos.x - halfSize.x > right ||
                        worldPos.y + halfSize.y < bottom || worldPos.y - halfSize.y > top) {
                        continue;
                    }
                }
                Vector2D finalScale = { 1.0f, 1.0f };
                //float finalRotation = 0.0f;
                // --- Apply Scale ---
                if (entityMgr.HasComponent(entity, ScaleType)) {
                    const Scale& scale = entityMgr.GetComponent<Scale>(entity);
                    finalScale = scale.axes;
                    halfSize.x *= finalScale.x;
                    halfSize.y *= finalScale.y;

                }
                // --- Rotation (degrees → radians) ---
                float rotationRad = 0.0f;
                bool hasRotationFlag = entityMgr.HasComponent(entity, RotationFlagType);
                bool hasRotationType = entityMgr.HasComponent(entity, RotationType);

                if (hasRotationFlag && hasRotationType) {
                    const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    const Rotation& rotation = entityMgr.GetComponent<Rotation>(entity);
                    float worldRotationRad = rotation.textureAngle;
                    float localRotationRad = static_cast<float>(rotationFlag.currentRotation * (M_PI / 180.0));
                    rotationRad = worldRotationRad + localRotationRad;
                }
                else if (hasRotationFlag) {
                    const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    rotationRad = static_cast<float>(rotationFlag.currentRotation * (M_PI / 180.0));
                }
                else if (hasRotationType) {
                    const Rotation& rotation = entityMgr.GetComponent<Rotation>(entity);
                    rotationRad = rotation.textureAngle;
                }

                // --- Fetch Texture Resource ---
                Texture& tex = resources_.get<Texture>(sprite.textureKey);

                //// --- Determine Render Order ---
                //int renderOrder = 0;
                //if (entityMgr.HasComponent(entity, BackgroundTagType))
                //    renderOrder = -100;
                //else if (isUIElement)
                //    renderOrder = 200;
                //else if (entityMgr.HasComponent(entity, PlayerTagType))
                //    renderOrder = 100;
                //else if (entityMgr.HasComponent(entity, EnemyTagType))
                //    renderOrder = 75;
                //else if (entityMgr.HasComponent(entity, PlatformTagType))
                //    renderOrder = 50;

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
                if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
                    // Convert center position from world to screen
                    screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);
                    
                    // Get camera for zoom and aspect ratio
                    const Camera& cam = cameraSystem_->GetCamera(activeCamera);

                    // Convert world-space size to screen-space size
                    // The key is to scale by the camera's zoom and aspect ratio
                    float halfWidth = cam.zoom * cam.aspectRatio;
                    float halfHeight = cam.zoom;

                    screenHalfSize.x = halfSize.x / halfWidth;
                    screenHalfSize.y = halfSize.y / halfHeight;
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

                // --- Add to Batch ---
                BatchKey key{ tex, renderOrder };
                batches[key].push_back(batchedSprite);
            }
            catch (const std::exception& e) {
                Logger::Get().Error(DebugType::GraphicsSystem)
                    << "Failed to render entity " << entity
                    << " (Bad key: " << sprite.textureKey << "): " << e.what();
                continue;
            }
        }

        // --- Render Sprite Batches ---
        for (auto& [key, sprites] : batches) {
            if (!sprites.empty()) {
                gfx_.drawSpriteBatch(key.texture, sprites);
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
        EntityManager entityMgr(ecs_);
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(AABBColliderType)) {
            if (!entityMgr.HasComponent(e, SceneOwnedType)) continue;
            const SceneOwned& sceneOwned = entityMgr.GetComponent<SceneOwned>(e);
            if (sceneOwned.sceneName != SceneManager::GetInstance().GetActiveScene().name) continue;

            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(e);
            if (!entityMgr.HasComponent(e, PositionType)) continue;

            const Vector2D& worldPos = entityMgr.GetComponent<Position>(e).Pos;
            const Vector2D& halfSize = collider.HalfSize;

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
            if (entityMgr.HasComponent(e, ScaleType)) {
                const Scale& scale = entityMgr.GetComponent<Scale>(e);
                scaledHalfSize.x *= scale.axes.x;
                scaledHalfSize.y *= scale.axes.y;
            }
            // Get rotation if entity has it
            float deg = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
                deg = entityMgr.GetComponent<RotationFlag>(e).currentRotation;
            }
            if (entityMgr.HasComponent(e, RotationType)) {
                deg += entityMgr.GetComponent<Rotation>(e).colliderAngle * (180.0f / 3.14159265359f);
            }

            // Use the rotated AABB calculation
            AABB_Bounds rotatedBounds = PhysicsSystem::GetRotatedAABB(worldPos, halfSize, deg);

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