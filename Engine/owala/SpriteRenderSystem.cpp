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

        // Iterate all entities that have a sprite component
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(TextureComponentType)) {
            TextureComponent& sprite = entityMgr.GetComponent<TextureComponent>(e);
            // Only require a position now
            if (!entityMgr.HasComponent(e, PositionType))
                continue;

            // Check if Entity is active in the scene. Loads if active.
            if (!entityMgr.HasComponent(e, ActiveTagType)) continue;

            const auto& pos = entityMgr.GetComponent<Position>(e).Pos;

            // Determine halfSize:
            Vector2D halfSize;
            if (entityMgr.HasComponent(e, AABBColliderType)) {
                halfSize = entityMgr.GetComponent<AABBCollider>(e).HalfSize;  // use physics size if available
            }
            else if (entityMgr.HasComponent(e, BackgroundTagType)) {
                // Fallback size for non-physics sprites (backgrounds/UI/etc.)
                // Adjust these values to your world scale
                halfSize = sprite.HalfSize;
            }
            else {
                // FOR TESTING THE DAMN 2500 SPRITES FAKE PLAYER 
                ////generate random seed for random size
                //std::random_device rd;
                //std::mt19937 gen(rd());
                //std::uniform_real_distribution<float> distribX(0.01f, 0.05f);
                //std::uniform_real_distribution<float> distribY(0.01f, 0.05f);

                //Vector2D randomSize = { distribX(gen), distribY(gen) };
                halfSize = sprite.HalfSize;
            }

            // === FRUSTUM CULLING CHECK ===
            // Calculate sprite bounds in screen space
            float left = pos.x - halfSize.x;
            float right = pos.x + halfSize.x;
            float bottom = pos.y - halfSize.y;
            float top = pos.y + halfSize.y;

            // Check if sprite is completely outside screen bounds (-1 to 1)
            if (right < -1.0f || left > 1.0f || bottom < -1.0f || top > 1.0f) {
                continue; // Skip rendering - sprite is offscreen
            }
            // === END CULLING CHECK ===

            // Optional rotation (degrees -> radians)
            float rotationRad = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
                rotationRad = static_cast<float>(entityMgr.GetComponent<RotationFlag>(e).currentRotation * (M_PI / 180.0f));
            }

            // Fetch the texture by key from ResourceManager
            // NOTE: your ResourceManager::get<T>(key) returns a T (by value) in your examples.
            // If it returns a shared_ptr or a ref in your impl, adjust accordingly.
            Texture& tex = resources_.get<Texture>(sprite.textureKey);

            // Convert center + halfSize to topLeft + fullSize for easy drawing
            Vector2D topLeft = { pos.x - halfSize.x, pos.y - halfSize.y };
            Vector2D fullSize = { halfSize.x * 2, halfSize.y * 2 };

            //Check for animation component
            if (entityMgr.HasComponent(e, AnimationComponentType)) {
                const AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(e);

                gfx_.drawSpriteEasy(tex, topLeft, fullSize, rotationRad, animComp.minUV, animComp.maxUV);
                continue; // Skip the non-animated draw call below
            }

            // Draw
            gfx_.drawSpriteEasy(tex, topLeft, fullSize, rotationRad);
        }

        // --- Render Text ---
        // This loop now handles text wrapping
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TextComponentType)) {
            const TextComponent& component = entityMgr.GetComponent<TextComponent>(entity);
            if (entityMgr.HasComponent(entity, PositionType)) {
                Font& font = resources_.get<Font>(component.fontName);
                // logger not yet added
                /*if (font == nullptr) {
                            logger.Info(DebugType::GraphicsSystem, "Font not found!!! " + fontName);
                            continue;
                        }*/
                Vector2D& original_pos = entityMgr.GetComponent<Position>(entity).Pos;

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
        EntityManager entityMgr(ecs_);
        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(AABBColliderType)) {
            const AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(e);
            if (!entityMgr.HasComponent(e, PositionType)) continue;

            const Vector2D& pos = entityMgr.GetComponent<Position>(e).Pos;
            const Vector2D& halfSize = collider.HalfSize;

            float deg = 0.0f;
            if (entityMgr.HasComponent(e, RotationFlagType)) {
                deg = entityMgr.GetComponent<RotationFlag>(e).currentRotation;
            }

            // Compute corners of the collider
            if (std::abs(deg) < 0.001f) {
                // Non-rotated: simple AABB
                Vector2D corners[4] = {
                    {pos.x - halfSize.x, pos.y - halfSize.y}, // bottom-left
                    {pos.x + halfSize.x, pos.y - halfSize.y}, // bottom-right
                    {pos.x + halfSize.x, pos.y + halfSize.y}, // top-right
                    {pos.x - halfSize.x, pos.y + halfSize.y}  // top-left
                };

                // Draw wireframe (4 lines connecting the corners)
                for (int i = 0; i < 4; ++i) {
                    int next = (i + 1) % 4;
                    gfx_.drawLine(corners[i], corners[next], { 0.0f, 1.0f, 0.0f, 1.0f }); // Green wireframe
                }
            }
            else {
                // Rotated: compute rotated corners
                float r = deg * (float)M_PI / 180.0f;
                float c = std::cos(r);
                float s = std::sin(r);
                Vector2D corners[4] = {
                    {-halfSize.x, -halfSize.y},
                    { halfSize.x, -halfSize.y},
                    { halfSize.x,  halfSize.y},
                    {-halfSize.x,  halfSize.y}
                };

                // Rotate and translate each corner
                Vector2D worldCorners[4];
                for (int i = 0; i < 4; ++i) {
                    worldCorners[i].x = corners[i].x * c - corners[i].y * s + pos.x;
                    worldCorners[i].y = corners[i].x * s + corners[i].y * c + pos.y;
                }

                // Draw wireframe (4 lines connecting the corners)
                for (int i = 0; i < 4; ++i) {
                    int next = (i + 1) % 4;
                    gfx_.drawLine(worldCorners[i], worldCorners[next], { 0.0f, 1.0f, 0.0f, 1.0f }); // Green wireframe
                }
            }
        }
    }
} // namespace Framework