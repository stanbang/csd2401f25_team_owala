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
            // Only require a position now
            if (!ecs_.position.count(e))
                continue;

            const auto& pos = ecs_.position.at(e).Pos;

            // Determine halfSize:
            Vector2D halfSize;
            if (ecs_.colliders.count(e)) {
                halfSize = ecs_.colliders.at(e).HalfSize;  // use physics size if available
            }
            else {
                // Fallback size for non-physics sprites (backgrounds/UI/etc.)
                // Adjust these values to your world scale
                halfSize = { 1.0f, 1.0f };
            }

            // Optional rotation (degrees -> radians)
            float rotationRad = 0.0f;
            if (ecs_.rotationFlag.count(e)) {
                rotationRad = ecs_.rotationFlag.at(e).currentRotation * (M_PI / 180.0f);
            }

            // Fetch the texture by key from ResourceManager
            // NOTE: your ResourceManager::get<T>(key) returns a T (by value) in your examples.
            // If it returns a shared_ptr or a ref in your impl, adjust accordingly.
            Texture& tex = resources_.get<Texture>(sprite.textureKey);

            // Draw
            gfx_.drawSprite(tex, pos, halfSize, rotationRad);
        }

        // --- Render Text ---
        // This loop now handles text wrapping
        for (const auto& [entity, component] : ecs_.text) {
            if (ecs_.position.count(entity)) {
                Font& font = resources_.get<Font>(component.fontName);
                // logger not yet added
                /*if (font == nullptr) {
                            logger.Info(DebugType::GraphicsSystem, "Font not found!!! " + fontName);
                            continue;
                        }*/
                const auto& original_pos = ecs_.position.at(entity).Pos;

                // --- NEW WRAPPING LOGIC ---

                // Define the maximum width for wrapping in NDC (-1.0 to 1.0).
                const float max_width_ndc = 1.8f;
                // Convert NDC width to pixel width for our wrapper function.
                const float max_width_pixels = max_width_ndc * 0.5f * gfx_.getScreenWidth();

                // Call the new WrapText function to get the lines.
                std::vector<std::string> lines = gfx_.WrapText(font, component.textToDraw, component.scale, max_width_pixels);

                // Render each line, moving down for each new line.
                Vector2D current_pos = original_pos;
                const float line_height_ndc = 0.08f; // Vertical distance between lines in NDC. Adjust as needed.

                for (const auto& line : lines) {
                    gfx_.drawText(font, line, current_pos, component.scale, component.color);
                    current_pos.y -= line_height_ndc; // Move down for the next line
                }
            }
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
