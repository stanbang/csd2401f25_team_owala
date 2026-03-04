/*********************************************************************//**
* \file   AISystem.h
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Declares the AISystem class, which orchestrates the AI logic updates,
* manages the navigation grid, and integrates AI components with the ECS.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h"
#include "FSMFactory.h" 

#ifdef BUILD_EDITOR
#include "Editor.h"
#include "imgui.h"
#endif

namespace Framework {

    class AISystem : public ISystem {
    public:
        AISystem() = default;

        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        inline void ForceGridRebuild() { navGrid_.BuildGrid(); }

#ifdef BUILD_EDITOR
        void DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse);
        bool debugVisualization = false;
        bool showNavigationGrid = false;
        bool showBirdDebug = false;
#endif

    private:
        NavigationGrid navGrid_;
        // The factory must be a member so it can share the NavGrid with states
        std::unique_ptr<FSMFactory> fsmFactory_;

        float gridRebuildTimer_ = 0.0f;
        const float gridRebuildInterval_ = 2.0f;

        Entity GetPlayer();

        // Helper to initialize FSMs for new entities
        void InitializeEntityFSM(Entity entity, FSMComponent& fsmComp, EntityStorage& ecs);

        // Debug helpers...
#ifdef BUILD_EDITOR
        //void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        // Updated to use FSMComponent
        void DrawFSMDebug(Entity entity, const FSMComponent& fsmComp, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
#endif
    };
}