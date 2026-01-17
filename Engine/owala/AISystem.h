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
#include "Graphics.h"
#include "FSMFactory.h"
#include <memory>
#include "TransformationSystem.h" 

#ifdef BUILD_EDITOR
struct ImDrawList;
namespace Editor {
    class Editor;
}
namespace Framework {
    class CameraSystem;
}
#endif

namespace Framework {

    class AISystem : public ISystem {
    public:
        AISystem(EntityStorage& storage, Graphics& gfx, TransformationSystem& transform);

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        NavigationGrid& GetNavGrid() { return navGrid_; }

#ifdef BUILD_EDITOR
        // Debug drawing
        void DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList,
            Editor::Editor* editor, Entity cameraToUse);

        bool debugVisualization = false;
        bool showNavigationGrid = false;
        bool showPaths = false;
        bool showColliders = false;
#endif

    private:
        EntityStorage* ecs_;
        NavigationGrid navGrid_;
        Graphics* gfx_;
        TransformationSystem* transformSystem_; 

        std::unique_ptr<FSMFactory> fsmFactory_;

        float gridRebuildTimer_ = 0.0f;
        const float gridRebuildInterval_ = 2.0f;

        float rotationPauseTimer_ = 0.0f; 
        bool wasPaused_ = false;          

#ifdef BUILD_EDITOR
        // Debug drawing helpers
        void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawPath(const std::vector<Vector2D>& path, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        void DrawCircle(const Vector2D& center, float radius, const Color& color,
            ImDrawList* drawList, Editor::Editor* editor, Entity cameraID, int segments = 32);
#endif
    };

} // namespace Framework