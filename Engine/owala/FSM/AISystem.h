#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Pathfinding.h"
#include "Components.h"
#include "FSMFactory.h" // Include your Factory header

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
        void DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
        // Updated to use FSMComponent
        void DrawFSMDebug(Entity entity, const FSMComponent& fsmComp, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID);
#endif
    };
}