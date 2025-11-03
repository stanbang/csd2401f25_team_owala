#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h" // Includes the BirdAIComponent and BirdState
#include "Graphics.h" 
#include <vector>     
#include <map> // For tracking state changes

// Forward declarations - only needed in editor build
#ifdef BUILD_EDITOR
struct ImDrawList;
namespace Editor { 
    class Editor; // forward declaration for editor
}

namespace Framework {
    class CameraSystem; // forward declare camera system
}
#endif

namespace Framework {

    class AISystem : public ISystem {
    public:
        AISystem(EntityStorage& storage, Graphics& gfx);

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        #ifdef BUILD_EDITOR
        // Debug drawing - ONLY available in editor build
        void DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse);

        // Public Toggles for the Editor
        bool debugVisualization = true;
        bool showNavigationGrid = false;
        bool showBirdDebug = true;
        bool showPaths = true;
        bool showColliders = true;
        #endif

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
        void HandleWakingState(Entity bird, BirdAIComponent& ai);
        void HandleChaseState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos);
        void HandleHoverState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos);
        void HandleAttackState(Entity bird, BirdAIComponent& ai);

        /**
         * @brief Moves the bird along its calculated path.
         */
        void FollowPath(Entity bird, BirdAIComponent& ai, Vector2D birdPos);

       
         #ifdef BUILD_EDITOR
        // ---- Private debug drawing helpers ---
         // Debug drawing helpers - ONLY compiled in editor build
        void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawBirdDebug(const Vector2D& birdPos, const BirdAIComponent& ai, const Vector2D& playerPos,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawPath(const std::vector<Vector2D>& path, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawCircle(const Vector2D& center, float radius, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID, int segments = 32);
        void DrawArrow(const Vector2D& start, const Vector2D& end, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        #endif
    };

} // namespace Framework

