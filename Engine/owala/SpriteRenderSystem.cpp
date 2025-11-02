#include "SpriteRenderSystem.h"
#include "EntityManager.h"
#include "Matrix3x3.h"
#include <cmath>
#include <random>
#include <algorithm> // Add this for std::sort
#include <tuple>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    void SpriteRenderSystem::Update() {
        // Clear frame once per frame
        gfx_.beginFrame(clearColor_);
        RenderSprites();
        if (drawColliders_) {
            RenderColliderOutlines();
        }
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

        // If we have an active camera, get its view bounds
        if (activeCamera != 0 && cameraSystem_) {
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        }

        // === BATCH COLLECTION ===
        // Use a struct to group by texture AND render order
        struct BatchKey {
            Texture texture;
            int renderOrder;

            bool operator<(const BatchKey& other) const {
                // Sort by renderOrder first, THEN by texture
                return std::tie(renderOrder, texture) < std::tie(other.renderOrder, other.texture);
            }
        };

        std::map<BatchKey, std::vector<BatchedSprite>> batches;

        // Iterate all entities that have a sprite component
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextureComponentType, PositionType, ActiveTagType)) {
            TextureComponent& sprite = entityMgr.GetComponent<TextureComponent>(entity);
            if (sprite.textureKey.empty()) continue;

            try {
                const Vector2D& worldPos = entityMgr.GetComponent<Position>(entity).Pos;

                // World-space half size
                Vector2D halfSize;
                if (entityMgr.HasComponent(entity, AABBColliderType)) {
                    halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
                }
                else if (entityMgr.HasComponent(entity, BackgroundTagType)) {
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
                if (entityMgr.HasComponent(entity, RotationFlagType)) {
                    rotationRad = static_cast<float>(entityMgr.GetComponent<RotationFlag>(entity).currentRotation * (M_PI / 180.0));
                }

                // Fetch the texture by key from ResourceManager
                Texture& tex = resources_.get<Texture>(sprite.textureKey);

                // === DETERMINE RENDER ORDER ===
                int renderOrder = 0; // Default order
                bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

                // Background gets lowest priority
                if (entityMgr.HasComponent(entity, BackgroundTagType)) {
                    renderOrder = -100;
                }
				else if (isUIElement) {
					renderOrder = 200; // UI elements render on top
				}
                // Players and important objects get higher priority
                else if (entityMgr.HasComponent(entity, PlayerTagType)) {
                    renderOrder = 100;
                }
                // Platforms and normal objects get medium priority
                else if (entityMgr.HasComponent(entity, PlatformTagType)) {
                    renderOrder = 50;
                }
                // Enemies get higher priority than platforms but lower than players
                else if (entityMgr.HasComponent(entity, EnemyTagType)) {
                    renderOrder = 75;
                }

                // === WORLD -> SCREEN TRANSFORM ===
                Vector2D screenPos = worldPos;
                Vector2D screenHalfSize = halfSize;

                if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
                    // Position to screen
                    screenPos = cameraSystem_->WorldToScreen(worldPos, activeCamera);

                    // Derive per-axis screen scale
                    const Vector2D sx = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 1.0f, 0.0f }, activeCamera) - screenPos;
                    const Vector2D sy = cameraSystem_->WorldToScreen(worldPos + Vector2D{ 0.0f, 1.0f }, activeCamera) - screenPos;

                    screenHalfSize.x = std::abs(sx.x) * halfSize.x;
                    screenHalfSize.y = std::abs(sy.y) * halfSize.y;
                }

                // Convert center + half to topLeft + full in SCREEN SPACE
                Vector2D topLeft = { screenPos.x - screenHalfSize.x, screenPos.y - screenHalfSize.y };
                Vector2D fullSize = { screenHalfSize.x * 2.0f,       screenHalfSize.y * 2.0f };

                // Create batched sprite
                BatchedSprite batchedSprite;
                batchedSprite.topLeft = topLeft;
                batchedSprite.size = fullSize;
                batchedSprite.rotation = rotationRad;

                // Get UV coordinates for animation
                if (entityMgr.HasComponent(entity, AnimationComponentType)) {
                    const AnimationComponent& animComp = entityMgr.GetComponent<AnimationComponent>(entity);
                    batchedSprite.minUV = animComp.minUV;
                    batchedSprite.maxUV = animComp.maxUV;
                }
                else {
                    batchedSprite.minUV = { 0.0f, 0.0f };
                    batchedSprite.maxUV = { 1.0f, 1.0f };
                }

                // Add to batch with render order
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

        // === RENDER BATCHES IN ORDER ===
        // std::map automatically sorts by key (which includes renderOrder)
        for (auto& [key, sprites] : batches) {
            if (!sprites.empty()) {
                gfx_.drawSpriteBatch(key.texture, sprites);
            }
        }

        // --- Render Text (always on top) ---
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextComponentType, PositionType, ActiveTagType)) {
            const TextComponent& component = entityMgr.GetComponent<TextComponent>(entity);
            if (component.fontKey.empty()) continue;

            try {
                Font& font = resources_.get<Font>(component.fontKey);
                const Vector2D& worldTextPos = entityMgr.GetComponent<Position>(entity).Pos;
                bool isUIElement = entityMgr.HasComponent(entity, GUITagType);

                Vector2D screenTextPos = worldTextPos;
                if (activeCamera != 0 && cameraSystem_ && !isUIElement) {
                    screenTextPos = cameraSystem_->WorldToScreen(worldTextPos, activeCamera);
                }

                const float max_width_ndc = 1.8f;
                const float max_width_pixels = max_width_ndc * 0.5f * gfx_.getScreenWidth();
                std::vector<std::string> lines = gfx_.WrapText(font, component.textToDraw, component.scale, max_width_pixels);

                Vector2D current_pos = screenTextPos;
                const float line_height_ndc = 0.08f;

                for (const auto& line : lines) {
                    gfx_.drawText(font, line, current_pos, component.scale, component.color);
                    current_pos.y -= line_height_ndc;
                }
            }

			catch (const std::exception& e) {
				Logger::Get().Error(DebugType::GraphicsSystem)
					<< "Failed to render text for entity " << entity
					<< " (Bad font key: " << component.fontKey << "): " << e.what();
				continue;
			}
        }
    }

    void SpriteRenderSystem::RenderColliderOutlines() {
        // This remains unchanged from your original implementation
        EntityManager entityMgr(ecs_);

        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        for (const Entity& e : entityMgr.GetAllEntitiesWithComponent(AABBColliderType)) {
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