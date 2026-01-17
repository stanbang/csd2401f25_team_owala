/*********************************************************************//**
* \file   AISystem.cpp
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
*
* \brief
* Implements the AISystem, containing the main update loop and logic for
* all AI state machines, like the Bird and Skeleton.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "AISystem.h"
#include "EntityManager.h"
#include "AIStates.h"
#include "Logger.h"
#include "OwalaTime.h"
#include "GameManager.h"


#ifdef BUILD_EDITOR
#include "Editor.h"
#include "CameraSystem.h"
#include "imgui.h"
#endif

namespace Framework {

    AISystem::AISystem(EntityStorage& storage, Graphics& gfx, TransformationSystem& transform)
        : ecs_(&storage), gfx_(&gfx), transformSystem_(&transform) {}

    void AISystem::Initialize() {
        navGrid_.BuildGrid(*ecs_);
        fsmFactory_ = std::make_unique<FSMFactory>(&navGrid_);

        Logger::Get().Info(DebugType::AISystem)
            << "AISystem initialized. Navigation grid built." << std::endl;
    }

    void AISystem::Shutdown() {
        Logger::Get().Info(DebugType::AISystem)
            << "AISystem shut down." << std::endl;
    }

    void AISystem::Update() {
        EntityManager entityMgr(*ecs_);
        float dt = Framework::Time::DeltaTime();

        if (transformSystem_->IsGlobalRotating()) {
            rotationPauseTimer_ = 1.0f; // Reset buffer to 1 second while rotating
            wasPaused_ = true;
        }

        if (rotationPauseTimer_ > 0.0f) {
            rotationPauseTimer_ -= dt;

            // While paused, force all AI entities to stop moving
            for (Entity entity : entityMgr.GetAllEntitiesWithComponent(FSMComponentType)) {
                if (entityMgr.HasComponent(entity, VelocityType)) {
                    entityMgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
                }
            }

            // If timer just finished, Force Rebuild the Grid immediately
            if (rotationPauseTimer_ <= 0.0f && wasPaused_) {
                Logger::Get().Info(DebugType::AISystem) << "Rotation complete. Rebuilding NavGrid..." << std::endl;
                navGrid_.BuildGrid(*ecs_); // <--- KEY FIX: Redraw grid now!
                wasPaused_ = false;
            }

            // SKIP FSM UPDATES while paused
            return;
        }

        // Rebuild grid periodically
        if (!entityMgr.HasComponent(player, TransformType)) {
            Logger::Get().Error(DebugType::AISystem) << "Player doesn't have Transform component!" << std::endl;
            return;
        }

        Vector2D playerPos = entityMgr.GetComponent<Transform>(player).position;
        

        // Get players collision halfsize
        Vector2D playerHalfSize = Vector2D::Zero();
        if (entityMgr.HasComponent(player, TextureComponentType)) { // Get visual size
            playerHalfSize = entityMgr.GetComponent<TextureComponent>(player).HalfSize;
        }



        // Rebuild grid periodically in case platforms move
        gridRebuildTimer_ += Framework::Time::DeltaTime();
        if (gridRebuildTimer_ >= gridRebuildInterval_) {
            navGrid_.BuildGrid();
            gridRebuildTimer_ = 0.0f;
        }

        // Update all entities with FSM components
        for (Entity entity : entityMgr.GetAllEntitiesWithComponents(FSMComponentType, ActiveTagType)) {
            FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(entity);

            // Initialize FSM on first frame
            if (!fsmComp.initialized) {
                if (fsmComp.stateMachine.GetCurrentState().empty()) {
                    // FSM hasn't been built yet, try to load it
                    std::string fsmPath = "Data/" + fsmComp.fsmType + ".json";
                    fsmComp.stateMachine = fsmFactory_->CreateFromFile(fsmPath);
                
                
                if (fsmComp.stateMachine.GetCurrentState().empty()) {
                    // Try to manually force initial state if file load failed but states exist
                    fsmComp.stateMachine.SetInitialState("Idle");
                }

               
            }

            fsmComp.stateMachine.Initialize(entity, *ecs_);
            fsmComp.initialized = true;

            // [DEBUG] Check if initialization worked
            if (fsmComp.stateMachine.GetCurrentState().empty()) {
                Logger::Get().Error(DebugType::AISystem)
                    << "FSM Initialization FAILED for entity " << entity
                    << ". Ensure " << fsmComp.fsmType << ".json exists and has 'initialState': 'Idle'" << std::endl;
            }
        }

            // Update FSM
            fsmComp.stateMachine.Update(entity, *ecs_, Framework::Time::DeltaTime());
        }
    }

                

#ifdef BUILD_EDITOR
    // Debug drawing implementation stays the same but references FSMComponent
    void AISystem::DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList,
        Editor::Editor* editor, Entity cameraToUse) {
        if (!debugVisualization || !drawList || !editor) return;

        EntityManager entityMgr(*ecs_);
        Entity activeCam = cameraToUse;
        if (activeCam == 0) return;

        float viewLeft, viewRight, viewBottom, viewTop;
        cameraSystem->GetViewBounds(activeCam, viewLeft, viewRight, viewBottom, viewTop);

        // Draw navigation grid
        if (showNavigationGrid) {
            DrawNavigationGrid(viewLeft, viewRight, viewBottom, viewTop,
                drawList, editor, activeCam);
        }

        // Draw paths for entities with Chase state
        if (showPaths) {
            for (Entity entity : entityMgr.GetAllEntitiesWithComponent(FSMComponentType)) {
                FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(entity);

                // Get the Chase state to access path
                if (auto* chaseState = dynamic_cast<BirdChaseState*>(
                    fsmComp.stateMachine.GetState("Chase"))) {
                    const auto& path = chaseState->GetCurrentPath();
                    if (!path.empty()) {
                        DrawPath(path, { 1.0f, 0.0f, 1.0f, 0.8f }, drawList, editor, activeCam);
                    }
                }
            }
        }

        // Draw colliders
        if (showColliders) {
            for (Entity entity : entityMgr.GetAllEntitiesWithComponents(
                FSMComponentType, AABBColliderType, PositionType)) {
                Vector2D pos = entityMgr.GetComponent<Position>(entity).Pos;
                Vector2D halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
                DrawAABB(pos, halfSize, { 0.0f, 1.0f, 0.0f, 0.5f }, drawList, editor, activeCam);
            }
        }
    }

    // Helper drawing functions remain the same
    inline ImU32 ToImColor(const Color& color) {
        return IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );
    }

    void AISystem::DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop,
        ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        Vector2D gridMin = navGrid_.WorldToGrid({ viewLeft, viewBottom });
        Vector2D gridMax = navGrid_.WorldToGrid({ viewRight, viewTop });

        Color walkableColor = { 0.0f, 1.0f, 0.0f, 0.2f };
        Color blockedColor = { 1.0f, 0.0f, 0.0f, 0.2f };
        ImU32 imWalkable = ToImColor(walkableColor);
        ImU32 imBlocked = ToImColor(blockedColor);

        for (int x = static_cast<int>(gridMin.x) - 1; x <= static_cast<int>(gridMax.x) + 1; ++x) {
            for (int y = static_cast<int>(gridMin.y) - 1; y <= static_cast<int>(gridMax.y) + 1; ++y) {
                Vector2D worldPos = navGrid_.GridToWorld(x, y);
                Vector2D min = { worldPos.x - navGrid_.cellSize * 0.5f,
                               worldPos.y - navGrid_.cellSize * 0.5f };
                Vector2D max = { worldPos.x + navGrid_.cellSize * 0.5f,
                               worldPos.y + navGrid_.cellSize * 0.5f };

                ImVec2 screenMin = editor->WorldToImGuiScreen(min, cameraID);
                ImVec2 screenMax = editor->WorldToImGuiScreen(max, cameraID);
                ImVec2 topLeft = { std::min(screenMin.x, screenMax.x),
                                 std::min(screenMin.y, screenMax.y) };
                ImVec2 botRight = { std::max(screenMin.x, screenMax.x),
                                  std::max(screenMin.y, screenMax.y) };

                drawList->AddRectFilled(topLeft, botRight,
                    navGrid_.IsWalkable(x, y) ? imWalkable : imBlocked);
            }
        }
    }

    void AISystem::DrawPath(const std::vector<Vector2D>& path, const Color& color,
        ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        if (path.size() < 2) return;

        ImU32 lineColor = ToImColor(color);
        for (size_t i = 0; i < path.size() - 1; ++i) {
            drawList->AddLine(
                editor->WorldToImGuiScreen(path[i], cameraID),
                editor->WorldToImGuiScreen(path[i + 1], cameraID),
                lineColor, 2.0f
            );
        }
    }

    void AISystem::DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color,
        ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        Vector2D min = center - halfSize;
        Vector2D max = center + halfSize;

        ImVec2 screenMin = editor->WorldToImGuiScreen(min, cameraID);
        ImVec2 screenMax = editor->WorldToImGuiScreen(max, cameraID);
        ImVec2 topLeft = { std::min(screenMin.x, screenMax.x),
                         std::min(screenMin.y, screenMax.y) };
        ImVec2 botRight = { std::max(screenMin.x, screenMax.x),
                          std::max(screenMin.y, screenMax.y) };

        drawList->AddRect(topLeft, botRight, ToImColor(color), 0.0f, 0, 2.0f);
    }

    void AISystem::DrawCircle(const Vector2D& center, float radius, const Color& color,
        ImDrawList* drawList, Editor::Editor* editor, Entity cameraID, int segments) {
        Vector2D worldPrevPoint = center + Vector2D(radius, 0);
        ImVec2 prevPoint = editor->WorldToImGuiScreen(worldPrevPoint, cameraID);
        ImU32 imColor = ToImColor(color);

        for (int i = 1; i <= segments; ++i) {
            float angle = (2.0f * 3.14159265f / segments) * i;
            Vector2D nextWorldPoint = center + Vector2D(
                std::cos(angle) * radius,
                std::sin(angle) * radius
            );
            ImVec2 nextPoint = editor->WorldToImGuiScreen(nextWorldPoint, cameraID);
            drawList->AddLine(prevPoint, nextPoint, imColor, 2.0f);
            prevPoint = nextPoint;
        }
    }
#endif

} // namespace Framework