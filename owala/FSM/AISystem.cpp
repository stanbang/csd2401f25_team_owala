/*********************************************************************//**
* \file   AISystem.cpp
*
* \primary_author
* Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
* Implements the AI System's initialization, update loop for processing
* entity FSMs, and debug drawing functionality for visualization.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "AISystem.h"
#include "EntityManager.h" 
#include "GameManager.h"
#include "Logger.h"
#include "OwalaTime.h"

#ifdef BUILD_EDITOR
#include "Editor.h"
#include "imgui.h"
#endif

namespace Framework {

    // Builds the navigation grid and creates the FSM factory. Called once at system startup.
    void AISystem::Initialize() {
        navGrid_.BuildGrid();
        // Initialize the factory with the navigation grid
        fsmFactory_ = std::make_unique<FSMFactory>(&navGrid_);

        Logger::Get().Info(DebugType::AISystem) << "AISystem initialized." << std::endl;
    }

    // Shuts down the AI system. The FSM factory cleans up its own resources.
    void AISystem::Shutdown() {
        // Factory cleans itself up
    }

    // Returns the first entity tagged as the player, or 0 if none is found.
    Entity AISystem::GetPlayer() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        auto players = entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType);
        return players.empty() ? 0 : players[0];
    }

    // Ticks every AI entity's FSM each frame.
    // Handles periodic nav-grid rebuilds, freezes AI during world rotation,
    // initializes newly spawned FSMs, and removes entities whose death animation has finished.
    void AISystem::Update() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Get all entities with FSM component
        std::vector<Entity> aiEntities = entityMgr.GetAllEntitiesWithComponent(true, FSMComponentType);

        bool worldIsRotating = GameManager::GetInstance().GetSystems().transformation_->IsGlobalRotating();

        // Grid Maintenance
        gridRebuildTimer_ += Framework::Time::DeltaTime();
        if (gridRebuildTimer_ >= gridRebuildInterval_ || worldIsRotating) {
            // Only rebuild if not rotating
            if (!worldIsRotating) {
                navGrid_.BuildGrid();
                gridRebuildTimer_ = 0.0f;
            }
        }

        // AI Update Loop
        float dt = Framework::Time::DeltaTime();
        std::vector<Entity> entitiesToDestroy;
        for (const Entity& entity : aiEntities) {
            FSMComponent& ai = entityMgr.GetComponent<FSMComponent>(entity);

            // STOP logic if world is rotating
            if (worldIsRotating) {
                if (entityMgr.HasComponent(entity, VelocityType)) {
                    entityMgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
                }
                continue;
            }

            // build fsm
            if (!ai.initialized) {
                InitializeEntityFSM(entity, ai, GameManager::GetInstance().GetECS());
            }

            //fsm update loop 
            ai.fsm.Update(entity, GameManager::GetInstance().GetECS(), dt);

            if (ai.fsm.GetCurrentState() == "Death") {
                State* deathState = ai.fsm.GetState("Death");
                if (deathState && deathState->IsFinished()) {
                    entitiesToDestroy.push_back(entity);
                }
            }
        }
        for (Entity entity : entitiesToDestroy) {
            entityMgr.DestroyEntity(entity);
        }
    }

    // Loads the FSM definition from JSON, initializes it, and applies any
    // serialized state or initial-state override stored on the component.
    void AISystem::InitializeEntityFSM(Entity entity, FSMComponent& fsmComp, EntityStorage& ecs) {
        if (fsmComp.fsmDefinitionPath.empty()) {
            Logger::Get().Error(DebugType::AISystem) << "Entity [" << entity << "] has FSMComponent but no definition path!" << std::endl;
            return;
        }

        // JSON logic
        fsmComp.fsm = fsmFactory_->CreateFromFile(fsmComp.fsmDefinitionPath);

        // Initialize 
        fsmComp.fsm.Initialize(entity, ecs);

        // Restore State 
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

    // debug drawings
#ifdef BUILD_EDITOR
    // Editor-only: iterates all AI entities and draws their FSM state label if debug visualization is on.
    void AISystem::DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse) {
        if (!debugVisualization) return;

        (void)cameraSystem;


        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        auto aiEntities = entityMgr.GetAllEntitiesWithComponent(true, FSMComponentType);

        for (Entity entity : aiEntities) {
            if (!entityMgr.HasComponent(entity, TransformType)) continue;

            FSMComponent& ai = entityMgr.GetComponent<FSMComponent>(entity);
            Vector2D pos = entityMgr.GetComponent<Transform>(entity).position;

            if (showBirdDebug) {
                DrawFSMDebug(entity, ai, pos, drawList, editor, cameraToUse);
            }
        }
    }

    // Editor-only: draws the current state name as a text label above a single AI entity.
    void AISystem::DrawFSMDebug(Entity entity, const FSMComponent& ai, const Vector2D& pos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        
        (void)entity;
        
        std::string stateName = ai.fsm.GetCurrentState();
        ImVec2 screenPos = editor->WorldToImGuiScreen(pos, cameraID);

        // Draw Text State Name above entity
        drawList->AddText(ImVec2(screenPos.x, screenPos.y - 40), IM_COL32(255, 255, 255, 255), stateName.c_str());

        // In progress
    }
#endif
}