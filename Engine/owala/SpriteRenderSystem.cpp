#include "SpriteRenderSystem.h"
#include "SpriteComponents.h"   // for ecs_.tex (textureKey)
#include "Matrix3x3.h"
#include <cmath>

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
        // Iterate all entities that have a sprite component
        for (const auto& [e, sprite] : ecs_.tex) {
            // Need a position and a collider (for half size)
            if (!ecs_.position.count(e) || !ecs_.colliders.count(e))
                continue;

            const auto& pos = ecs_.position.at(e).Pos;
            const auto& halfSize = ecs_.colliders.at(e).HalfSize;

            // Optional rotation (degrees -> radians)
            float rotationRad = 0.0f;
            if (ecs_.rotationFlag.count(e)) {
                rotationRad = ecs_.rotationFlag.at(e).currentRotation * (M_PI / 180.0f);
            }

            // Fetch the texture by key from ResourceManager
            // NOTE: your ResourceManager::get<T>(key) returns a T (by value) in your examples.
            // If it returns a shared_ptr or a ref in your impl, adjust accordingly.
            Texture tex = resources_.get<Texture>(sprite.textureKey);

            // Draw
            gfx_.drawSprite(tex, pos, halfSize, rotationRad);
        }
    }

    void SpriteRenderSystem::RenderColliderOutlines() {
        for (const auto& [e, collider] : ecs_.colliders) {
            if (!ecs_.position.count(e)) continue;

            const auto& pos = ecs_.position.at(e).Pos;
            const auto& halfSize = collider.HalfSize;

            float deg = 0.0f;
            if (ecs_.rotationFlag.count(e)) {
                deg = ecs_.rotationFlag.at(e).currentRotation;
            }

            // Compute AABB of the (optionally) rotated box for quick debug rendering
            if (std::abs(deg) < 0.001f) {
                Vector2D mn{ pos.x - halfSize.x, pos.y - halfSize.y };
                Vector2D mx{ pos.x + halfSize.x, pos.y + halfSize.y };
                gfx_.drawRectangle(mn, mx, { 0.8f, 0.8f, 0.8f, 1.0f });
            }
            else {
                float r = deg * (float)M_PI / 180.0f;
                float c = std::cos(r);
                float s = std::sin(r);
                Vector2D corners[4] = {
                    {-halfSize.x, -halfSize.y}, { halfSize.x, -halfSize.y},
                    { halfSize.x,  halfSize.y}, {-halfSize.x,  halfSize.y}
                };

                float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
                for (int i = 0; i < 4; ++i) {
                    float rx = corners[i].x * c - corners[i].y * s + pos.x;
                    float ry = corners[i].x * s + corners[i].y * c + pos.y;
                    if (rx < minX) minX = rx; if (rx > maxX) maxX = rx;
                    if (ry < minY) minY = ry; if (ry > maxY) maxY = ry;
                }
                gfx_.drawRectangle({ minX, minY }, { maxX, maxY }, { 0.8f, 0.8f, 0.8f, 1.0f });
            }
        }
    }

} // namespace Framework
