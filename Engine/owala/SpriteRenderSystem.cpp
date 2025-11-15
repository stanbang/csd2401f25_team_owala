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
     * \brief Renders all visible sprites and text components.
     *
     * Collects all entities with active `TextureComponent`, applies
     * world-to-screen transformations, performs frustum culling,
     * sorts by render order and texture, and draws batched sprites.
     * Then renders text components on top of all layers.
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

                // --- Rotation (degrees → radians) ---
                float rotationRad = 0.0f;
                if (entityMgr.HasComponent(entity, RotationFlagType)) {
                    rotationRad = static_cast<float>(entityMgr.GetComponent<RotationFlag>(entity).currentRotation * (M_PI / 180.0));
                }

                // --- Fetch Texture Resource ---
                Texture& tex = resources_.get<Texture>(sprite.textureKey);

                // --- Determine Render Order ---
                int renderOrder = 0;
                if (entityMgr.HasComponent(entity, BackgroundTagType))
                    renderOrder = -100;
                else if (isUIElement)
                    renderOrder = 200;
                else if (entityMgr.HasComponent(entity, PlayerTagType))
                    renderOrder = 100;
                else if (entityMgr.HasComponent(entity, EnemyTagType))
                    renderOrder = 75;
                else if (entityMgr.HasComponent(entity, PlatformTagType))
                    renderOrder = 50;

                // --- World → Screen Conversion ---
                Vector2D topLeft = { worldPos.x - halfSize.x, worldPos.y - halfSize.y };
                Vector2D fullSize = { halfSize.x * 2.0f, halfSize.y * 2.0f };

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

        //// --- Render Text ---
        //for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextComponentType, PositionType, ActiveTagType)) {
        //    const TextComponent& component = entityMgr.GetComponent<TextComponent>(entity);
        //    if (component.fontKey.empty()) continue;

        //    try {
        //        Font& font = resources_.get<Font>(component.fontKey);
        //        const Vector2D& worldTextPos = entityMgr.GetComponent<Position>(entity).Pos;
        //        bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

        //        Vector2D screenTextPos = worldTextPos;
        //        if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
        //            screenTextPos = cameraSystem_->WorldToScreen(worldTextPos, activeCamera);
        //        }

        //        const float max_width_ndc = 1.8f;
        //        const float max_width_pixels = max_width_ndc * 0.5f * gfx_.getScreenWidth();
        //        std::vector<std::string> lines = gfx_.WrapText(font, component.textToDraw, component.scale, max_width_pixels);

        //        Vector2D current_pos = screenTextPos;
        //        const float line_height_ndc = 0.08f;

        //        float zoom = 1.0f;
        //        if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
        //            const Camera& cam = cameraSystem_->GetCamera(activeCamera);
        //            zoom = cam.zoom;
        //        }

        //        for (const auto& line : lines) {
        //            gfx_.drawText(font, line, current_pos, component.scale / zoom, component.color);
        //            current_pos.y -= line_height_ndc / zoom;
        //        }
        //    }
        //    catch (const std::exception& e) {
        //        Logger::Get().Error(DebugType::GraphicsSystem)
        //            << "Failed to render text for entity " << entity
        //            << " (Bad font key: " << component.fontKey << "): " << e.what();
        //        continue;
        //    }
        //}
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
                const Vector2D sx = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 1.0f, 0.0f }, activeCamera) - screenPos;
                const Vector2D sy = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 0.0f, 1.0f }, activeCamera) - screenPos;

                screenHalfSize.x = std::abs(sx.x) * halfSize.x;
                screenHalfSize.y = std::abs(sy.y) * halfSize.y;
            }

            float deg = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
                deg = entityMgr.GetComponent<RotationFlag>(e).currentRotation;
            }

            if (std::abs(deg) < 0.001f) {
                Vector2D corners[4] = {
                    {screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y},
                    {screenPos.x + screenHalfSize.x, screenPos.y - screenHalfSize.y},
                    {screenPos.x + screenHalfSize.x, screenPos.y + screenHalfSize.y},
                    {screenPos.x - screenHalfSize.x, screenPos.y + screenHalfSize.y}
                };
                for (int i = 0; i < 4; ++i) {
                    int next = (i + 1) % 4;
                    gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f });
                }
            }
            else {
                const float r = deg * (float)M_PI / 180.0f;
                const float c = std::cos(r);
                const float s = std::sin(r);

                Vector2D local[4] = {
                    {-screenHalfSize.x, -screenHalfSize.y},
                    { screenHalfSize.x, -screenHalfSize.y},
                    { screenHalfSize.x,  screenHalfSize.y},
                    {-screenHalfSize.x,  screenHalfSize.y}
                };

                Vector2D corners[4];
                for (int i = 0; i < 4; ++i) {
                    corners[i].x = local[i].x * c - local[i].y * s + screenPos.x;
                    corners[i].y = local[i].x * s + local[i].y * c + screenPos.y;
                }

                for (int i = 0; i < 4; ++i) {
                    int next = (i + 1) % 4;
                    gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f });
                }
            }
        }
    }

} // namespace Framework
