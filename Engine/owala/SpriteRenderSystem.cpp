#include "SpriteRenderSystem.h"
#include "EntityManager.h"
#include "Matrix3x3.h"
#include <cmath>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    void SpriteRenderSystem::Update(float /*dt*/) {
        // Clear frame once per frame (do NOT also clear in GameSystem anymore)
        gfx_.beginFrame(clearColor_);
        RenderSprites();
        if (drawColliders_) {
            RenderColliderOutlines();
        }
        // Note: your Graphics system (added last) should still be the one that swaps buffers.
    }

    void SpriteRenderSystem::RenderSprites() {
        EntityManager entityMgr(ecs_);

        // === GET ACTIVE CAMERA ===
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        // Default view bounds if no camera
        float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;

        // If we have an active camera, get its view bounds (in WORLD SPACE)
        if (activeCamera != 0 && cameraSystem_) {
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        }

        // Iterate all entities that have a sprite component
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(TextureComponentType)) {
            TextureComponent& sprite = entityMgr.GetComponent<TextureComponent>(e);
            if (!entityMgr.HasComponent(e, PositionType)) continue;
            if (!entityMgr.HasComponent(e, ActiveTagType)) continue;

            const Vector2D& worldPos = entityMgr.GetComponent<Position>(e).Pos;

            // World-space half size (physics/authoring units)
            Vector2D halfSize;
            if (entityMgr.HasComponent(e, AABBColliderType)) {
                halfSize = entityMgr.GetComponent<AABBCollider>(e).HalfSize;
            }
            else if (entityMgr.HasComponent(e, BackgroundTagType)) {
                halfSize = sprite.HalfSize;
            }
            else {
                halfSize = sprite.HalfSize;
            }

            // === CULLING IN WORLD SPACE ===
            if (activeCamera != 0 && cameraSystem_) {
                if (worldPos.x + halfSize.x < left || worldPos.x - halfSize.x > right ||
                    worldPos.y + halfSize.y < bottom || worldPos.y - halfSize.y > top) {
                    continue;
                }
            }

            // Optional rotation (degrees -> radians)
            float rotationRad = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
                rotationRad = static_cast<float>(entityMgr.GetComponent<RotationFlag>(e).currentRotation * (M_PI / 180.0));
            }

            // Fetch the texture by key from ResourceManager
            Texture& tex = resources_.get<Texture>(sprite.textureKey);

            // === WORLD -> SCREEN TRANSFORM ===
            Vector2D screenPos = worldPos;
            Vector2D screenHalfSize = halfSize; // default if no camera

            if (activeCamera != 0 && cameraSystem_) {
                // Position to screen
                screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                // Derive per-axis screen scale for 1 world unit
                const Vector2D sx = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 1.0f, 0.0f }, activeCamera) - screenPos;
                const Vector2D sy = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 0.0f, 1.0f }, activeCamera) - screenPos;

                // Map world half-size to screen half-size (assumes axis-aligned camera)
                screenHalfSize.x = std::abs(sx.x) * halfSize.x;
                screenHalfSize.y = std::abs(sy.y) * halfSize.y;
            }

            // Convert center + half to topLeft + full in SCREEN SPACE (now zoom-aware)
            Vector2D topLeft = { screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y };
            Vector2D fullSize = { screenHalfSize.x * 2.0f,       screenHalfSize.y * 2.0f };

            // Draw
            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                const AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(e);
                gfx_.drawSpriteEasy(tex, topLeft, fullSize, rotationRad, animComp.minUV, animComp.maxUV);
            }
            else {
                gfx_.drawSpriteEasy(tex, topLeft, fullSize, rotationRad);
            }
        }

        // --- Render Text ---
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TextComponentType)) {
            const TextComponent& component = entityMgr.GetComponent<TextComponent>(entity);
            if (!entityMgr.HasComponent(entity, PositionType)) continue;

            Font& font = resources_.get<Font>(component.fontName);
            const Vector2D& worldTextPos = entityMgr.GetComponent<Position>(entity).Pos;

            // WORLD -> SCREEN for text position
            Vector2D screenTextPos = worldTextPos;
            if (activeCamera != 0 && cameraSystem_) {
                screenTextPos = cameraSystem_->WorldToScreen(worldTextPos, activeCamera);
            }

            // Wrapping in pixels (unchanged)
            const float max_width_ndc = 1.8f;
            const float max_width_pixels = max_width_ndc * 0.5f * gfx_.getScreenWidth();
            std::vector<std::string> lines = gfx_.WrapText(font, component.textToDraw, component.scale, max_width_pixels);

            // Render each line
            Vector2D current_pos = screenTextPos;
            const float line_height_ndc = 0.08f;

            for (const auto& line : lines) {
                gfx_.drawText(font, line, current_pos, component.scale, component.color);
                current_pos.y -= line_height_ndc;
            }
        }
    }


    void SpriteRenderSystem::RenderColliderOutlines() {
        EntityManager entityMgr(ecs_);

        // === GET ACTIVE CAMERA ===
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(AABBColliderType)) {
            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(e);
            if (!entityMgr.HasComponent(e, PositionType)) continue;

            const Vector2D& worldPos = entityMgr.GetComponent<Position>(e).Pos;
            const Vector2D& halfSize = collider.HalfSize;

            // WORLD -> SCREEN
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
                // Non-rotated AABB in screen space
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
                // Rotated rectangle in screen space
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