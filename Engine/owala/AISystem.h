/*********************************************************************//**
* \file   AISystem.h
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
*
* \brief
* Declares the AISystem class, which orchestrates all enemy AI behaviors 
* and manages their state machines.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h" // Includes the BirdAIComponent and Skeleton Component
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

    struct SkeletonAIComponent;

    class AISystem : public ISystem {
    public:
        AISystem(EntityStorage& storage, Graphics& gfx);

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        #ifdef BUILD_EDITOR
        // Debug drawing - ONLY available in editor build
        void DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse);

        // Public Toggles for the Editor for debug visualization
        bool debugVisualization = false;
        bool showNavigationGrid = false;
        bool showBirdDebug = false;
        bool showPaths = false;
        bool showColliders = false;
        #endif

    private:
        EntityStorage* ecs_;
        NavigationGrid navGrid_;
        AStarPather pather_;
        Graphics* gfx_; // Pointer to the graphics system
       // GameManager* gameManager_;

        float gridRebuildTimer_ = 0.0f;
        const float gridRebuildInterval_ = 2.0f;

        /**
         * @brief Finds the player entity (using PlayerTagType). Returns 0 if not found.
         */
        Entity GetPlayer();

        // --- State Machine Handlers for bird AI ---
        void HandleIdleState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos);
        void HandleWakingState(Entity bird, BirdAIComponent& ai);
        void HandleChaseState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D birdHalfSize, Vector2D playerPos, Vector2D playerHalfSize);
        void HandleHoverState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D birdHalfSize, Vector2D playerPos, Vector2D playerHalfSize);
        void HandleAttackState(Entity bird, BirdAIComponent& ai);
        void HandleStunnedState(Entity bird, BirdAIComponent& ai);

        // Helper for setting facing direction
        void UpdateBirdFacingDirection(Entity bird, const Vector2D& targetPos);

        // --- Skeleton State Machine Handlers --- (not in use currently)
        void HandleSkeletonIdleState(Entity skeleton, SkeletonAIComponent& ai, Vector2D skeletonPos, Vector2D playerPos);
        void HandleSkeletonChaseState(Entity skeleton, SkeletonAIComponent& ai, Vector2D skeletonPos, Vector2D playerPos);

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

