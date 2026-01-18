#include "AISystem.h"
#include "EntityManager.h" 
#include "GameManager.h"
#include "Logger.h"
#include "OwalaTime.h"

// [Include your specific states here if needed for Debug Drawing casts, otherwise not needed for logic]
#include "AIStates.h" 

#ifdef BUILD_EDITOR
#include "Editor.h"
#include "imgui.h"
#endif

namespace Framework {

    void AISystem::Initialize() {
        navGrid_.BuildGrid();
        // Initialize the factory with the navigation grid
        fsmFactory_ = std::make_unique<FSMFactory>(&navGrid_);

        Logger::Get().Info(DebugType::AISystem) << "AISystem initialized." << std::endl;
    }

    void AISystem::Shutdown() {
        // Factory cleans itself up
    }

    Entity AISystem::GetPlayer() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        auto players = entityMgr.GetAllEntitiesWithComponent(PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    void AISystem::Update() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Get all entities with FSM component
        std::vector<Entity> aiEntities = entityMgr.GetAllEntitiesWithComponent(FSMComponentType); // Make sure to add this Enum to Components.h

        bool worldIsRotating = GameManager::GetInstance().GetSystems().transformation_->IsGlobalRotating();

        // 1. Grid Maintenance
        gridRebuildTimer_ += Framework::Time::DeltaTime();
        if (gridRebuildTimer_ >= gridRebuildInterval_ || worldIsRotating) {
            // Only rebuild if not rotating, or logic to handle rotation
            if (!worldIsRotating) {
                navGrid_.BuildGrid();
                gridRebuildTimer_ = 0.0f;
            }
        }

        // 2. AI Update Loop
        float dt = Framework::Time::DeltaTime();

        for (const Entity& entity : aiEntities) {
            FSMComponent& ai = entityMgr.GetComponent<FSMComponent>(entity);

            // STOP logic if world is rotating
            if (worldIsRotating) {
                if (entityMgr.HasComponent(entity, VelocityType)) {
                    entityMgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
                }
                continue;
            }

            // Lazy Initialization: If FSM isn't built yet, build it now
            if (!ai.initialized) {
                InitializeEntityFSM(entity, ai, GameManager::GetInstance().GetECS());
            }

            // The Magic: Generic Update
            ai.fsm.Update(entity, GameManager::GetInstance().GetECS(), dt);
        }
    }

    void AISystem::InitializeEntityFSM(Entity entity, FSMComponent& fsmComp, EntityStorage& ecs) {
        if (fsmComp.fsmDefinitionPath.empty()) {
            Logger::Get().Error(DebugType::AISystem) << "Entity [" << entity << "] has FSMComponent but no definition path!" << std::endl;
            return;
        }

        // 1. Create Logic from JSON
        fsmComp.fsm = fsmFactory_->CreateFromFile(fsmComp.fsmDefinitionPath);

        // 2. Initialize (Call OnEnter for initial state)
        fsmComp.fsm.Initialize(entity, ecs);

        // 3. Restore State (if we loaded from a save file)
        if (!fsmComp.serializedState.empty()) {
            fsmComp.fsm.ChangeState(entity, ecs, fsmComp.serializedState);
            fsmComp.serializedState.clear(); // Clear so we don't force it again
        }
        else if (!fsmComp.initialStateOverride.empty()) {
            fsmComp.fsm.ChangeState(entity, ecs, fsmComp.initialStateOverride);
        }

        fsmComp.initialized = true;
        Logger::Get().Info(DebugType::AISystem) << "Initialized FSM for Entity [" << entity << "] from " << fsmComp.fsmDefinitionPath << std::endl;
    }

    // [KEEP DRAW DEBUG - Just Updated]
#ifdef BUILD_EDITOR
    void AISystem::DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse) {
        if (!debugVisualization) return;

        // ... [Keep your existing Grid drawing code] ...

        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        auto aiEntities = entityMgr.GetAllEntitiesWithComponent(FSMComponentType);

        for (Entity entity : aiEntities) {
            if (!entityMgr.HasComponent(entity, TransformType)) continue;

            FSMComponent& ai = entityMgr.GetComponent<FSMComponent>(entity);
            Vector2D pos = entityMgr.GetComponent<Transform>(entity).position;

            if (showBirdDebug) {
                DrawFSMDebug(entity, ai, pos, drawList, editor, cameraToUse);
            }
        }
    }

    void AISystem::DrawFSMDebug(Entity entity, const FSMComponent& ai, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        // We can't easily switch on state types anymore since they are generic classes.
        // We debug the *Current State Name* string instead.

        std::string stateName = ai.fsm.GetCurrentState();
        ImVec2 screenPos = editor->WorldToImGuiScreen(pos, cameraID);

        // Draw Text State Name above entity
        drawList->AddText(ImVec2(screenPos.x, screenPos.y - 40), IM_COL32(255, 255, 255, 255), stateName.c_str());

        // You can cast to specific states if you really need to draw specific radii
        // But for a generic system, visualizing the Grid and Path is usually enough.

        // If you need to access specific state data for debug (like BirdChaseState::path),
        // you would need to dynamic_cast the current state:
        /*
        State* currentState = const_cast<FSM&>(ai.fsm).GetState(stateName);
        if (auto* chase = dynamic_cast<BirdChaseState*>(currentState)) {
             DrawPath(chase->GetCurrentPath(), ...);
        }
        */
    }
#endif
}