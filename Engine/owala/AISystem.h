#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h" // Includes the BirdAIComponent and BirdState
#include "Graphics.h" 
#include <vector>     
#include <map> // For tracking state changes

namespace Framework {
    class CameraSystem; // forward declare camera system
}

namespace Framework {

    class AISystem : public ISystem {
    public:
        AISystem(EntityStorage& storage, Graphics& gfx);

        void Initialize() override;
        void Update(float dt) override;
        void Shutdown() override;

        // Main debug draw call (will be called by the Editor)
        void DrawDebug(CameraSystem* cameraSystem);

        // Public Toggles for the Editor to access
        bool debugVisualization = true;
        bool showNavigationGrid = false;
        bool showBirdDebug = true;
        bool showPaths = true;
        bool showColliders = true;

    private:
        EntityStorage* ecs_;
        NavigationGrid navGrid_;
        AStarPather pather_;
        Graphics* gfx_; // Pointer to the graphics system

        float gridRebuildTimer_ = 0.0f;
        const float gridRebuildInterval_ = 2.0f;

        /**
         * @brief Finds the player entity (using PlayerTagType). Returns 0 if not found.
         */
        Entity GetPlayer();

        // --- State Machine Handlers ---
        void HandleIdleState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos);
        void HandleWakingState(Entity bird, BirdAIComponent& ai, float dt);
        void HandleChaseState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos, float dt);
        void HandleHoverState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos, float dt);
        void HandleAttackState(Entity bird, BirdAIComponent& ai, float dt);

        /**
         * @brief Moves the bird along its calculated path.
         */
        void FollowPath(Entity bird, BirdAIComponent& ai, Vector2D birdPos, float dt);

        // ---- Private debug drawing helpers ---
        void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop);
        void DrawBirdDebug(const Vector2D& birdPos, const BirdAIComponent& ai, const Vector2D& playerPos);
        void DrawPath(const std::vector<Vector2D>& path, const Color& color);
        void DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color);
        void DrawCircle(const Vector2D& center, float radius, const Color& color, int segments = 32);
        void DrawArrow(const Vector2D& start, const Vector2D& end, const Color& color);

        //  Draw the 5 hover nodes in a circle
        void DrawHoverNodes(const Vector2D& playerPos, const BirdAIComponent& ai);
    };

} // namespace Framework

