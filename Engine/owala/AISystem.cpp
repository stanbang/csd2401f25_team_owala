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
#include "EntityStorage.h" // For ComponentType enums
#include <cmath>           // For std::abs and std::sin
#include <cstdlib>         // For rand()
#include <algorithm>       // For std::clamp
#include "Logger.h"         // Includes Logger
#include "CameraSystem.h"   // Includes CameraSystem
#include "OwalaTime.h"
#include "GameManager.h"


// Only include editor/ImGui headers in editor build
#ifdef BUILD_EDITOR
#include "Editor.h"
#include "imgui.h"
#endif


namespace Framework {

    AISystem::AISystem(EntityStorage& storage, Graphics& gfx)
        : ecs_(&storage), gfx_(&gfx) {}

    void AISystem::Initialize() {
        // Build the navigation grid 
        navGrid_.BuildGrid(*ecs_);
        Logger::Get().Info(DebugType::AISystem) << "AISystem initialized. Navigation grid built." << std::endl;
    }

    void AISystem::Shutdown() {
        // Nothing to clean up
        Logger::Get().Info(DebugType::AISystem) << "AISystem shut down." << std::endl;
    }

    Entity AISystem::GetPlayer() {
        EntityManager entityMgr(*ecs_);
        // get PlayerTagType component
        auto players = entityMgr.GetAllEntitiesWithComponent(PlayerTagType);
        if (!players.empty()) {
            return players[0];
        }
        return 0; // 0 is an invalid entity ID
    }

    void AISystem::Update() {
        EntityManager entityMgr(*ecs_);

        // Check if world is rotating
        bool worldIsRotating = false;
       
        worldIsRotating = GameManager::GetInstance().GetSystems().transformation_->IsGlobalRotating();
  

        // Rebuild grid on rotation
        static bool wasRotating = false;
        if (worldIsRotating && !wasRotating) {
            navGrid_.BuildGrid(*ecs_); // Force rebuild
            Logger::Get().Info(DebugType::AISystem)
                << "World rotation started - grid rebuilt" << std::endl;
        }
        wasRotating = worldIsRotating;


        // Find the player
        Entity player = GetPlayer();
        if (player == 0) {
            static bool playerWarningLogged = false;
            if (!playerWarningLogged) {
                Logger::Get().Info(DebugType::AISystem) << "No player found in scene!" << std::endl;
                playerWarningLogged = true;
            }
            return;
        }

        if (!entityMgr.HasComponent(player, PositionType)) {
            Logger::Get().Error(DebugType::AISystem) << "Player doesn't have Position component!" << std::endl;
            return;
        }

        Vector2D playerPos = entityMgr.GetComponent<Position>(player).Pos;
        

        // Get players collision halfsize
        Vector2D playerHalfSize = Vector2D::Zero();
        if (entityMgr.HasComponent(player, TextureComponentType)) { // Get visual size
            playerHalfSize = entityMgr.GetComponent<TextureComponent>(player).HalfSize;
        }



        // Rebuild grid periodically in case platforms move
        gridRebuildTimer_ += Framework::Time::DeltaTime();
        if (gridRebuildTimer_ >= gridRebuildInterval_) {
            navGrid_.BuildGrid(*ecs_);
            gridRebuildTimer_ = 0.0f;

            Logger::Get().Info(DebugType::AISystem) << "Navigation grid rebuilt." << std::endl;
        }

        // Loop over all entities with an EnemyTagType and BirdAIComponent
        for (const Entity& enemy : entityMgr.GetAllEntitiesWithComponent(EnemyTagType)) {
            // Check that the bird has all components before proceeding
            if (entityMgr.HasComponent(enemy, BirdAIComponentType))
            {
                if (!entityMgr.HasComponent(enemy, PositionType) ||
                    !entityMgr.HasComponent(enemy, VelocityType))
                {
                    continue; // Skip if bird doesn't have all required components
                }

                Framework::BirdAIComponent& ai = entityMgr.GetComponent<BirdAIComponent>(enemy);
                Vector2D& birdPos = entityMgr.GetComponent<Position>(enemy).Pos;

                // Get birds collision halfsize
                Vector2D birdHalfSize = Vector2D::Zero();
                /*if (entityMgr.HasComponent(enemy, AABBColliderType)) {
                    birdHalfSize = entityMgr.GetComponent<AABBCollider>(enemy).HalfSize;
                }*/

                if (entityMgr.HasComponent(enemy, TextureComponentType)) { // Get visual size
                    birdHalfSize = entityMgr.GetComponent<TextureComponent>(enemy).HalfSize;
                }

                // PAUSE AI during world rotation
                if (worldIsRotating) {
                    if (entityMgr.HasComponent(enemy, VelocityType)) {
                        entityMgr.GetComponent<Velocity>(enemy).Vel = Vector2D::Zero();
                    }
                    continue; // Skip AI updates
                }

#ifdef BUILD_EDITOR
                // Log state transitions - only in editor build
                static std::map<Entity, BirdState> lastStates;
                if (lastStates.find(enemy) == lastStates.end()) {
                    lastStates[enemy] = ai.state;
                }

                if (ai.state != lastStates[enemy]) {
                    std::string stateName;
                    switch (ai.state) {
                    case BirdState::IDLE:   stateName = "IDLE";   break;
                    case BirdState::WAKING: stateName = "WAKING"; break;
                    case BirdState::CHASE:  stateName = "CHASE";  break;
                    case BirdState::HOVER:  stateName = "HOVER";  break;
                    case BirdState::ATTACK: stateName = "ATTACK"; break;
                    }
                    Logger::Get().Info(DebugType::AISystem)
                        << "Bird [" << enemy << "] State Change: " << stateName << std::endl;
                    lastStates[enemy] = ai.state;
                }
#endif
                // --- Main State Machine ---
                switch (ai.state) {
                case BirdState::IDLE:
                    HandleIdleState(enemy, ai, birdPos, playerPos);
                    break;
                case BirdState::WAKING:
                    HandleWakingState(enemy, ai);
                    break;
                case BirdState::CHASE:
                    HandleChaseState(enemy, ai, birdPos, {}, playerPos, {});
                    break;
                case BirdState::ATTACK:
                    HandleAttackState(enemy, ai);
                    break;
                case BirdState::STUNNED:
                    HandleStunnedState(enemy, ai);
                    break;
                case BirdState::HOVER:
                    HandleHoverState(enemy, ai, birdPos, {}, playerPos, {});
                    break;
                }

            }
            else if (entityMgr.HasComponent(enemy, SkeletonAIComponentType))
            {
                // Skeletons need these components to function
                if (!entityMgr.HasComponent(enemy, PositionType) ||
                    !entityMgr.HasComponent(enemy, VelocityType) ||
                    !entityMgr.HasComponent(enemy, PlayerStateType)) // Used for facing and grounded
                {
                    continue;
                }

                auto& ai = entityMgr.GetComponent<SkeletonAIComponent>(enemy);
                auto& skeletonPos = entityMgr.GetComponent<Position>(enemy).Pos;

                // --- Skeleton State Machine ---
                switch (ai.state) {
                case SkeletonState::IDLE:
                    HandleSkeletonIdleState(enemy, ai, skeletonPos, playerPos);
                    break;
                case SkeletonState::CHASE:
                    HandleSkeletonChaseState(enemy, ai, skeletonPos, playerPos);
                    break;
                }
            }
        }

    }

    void AISystem::HandleIdleState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos) {
        EntityManager entityMgr(*ecs_);
        if (!entityMgr.HasComponent(bird, VelocityType)) {
            Logger::Get().Error(DebugType::AISystem) << "Bird Enemy is missing Velocity component!" << std::endl;
            return;
        }

        entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
        // TODO: Trigger Sleeping animation here
        
        // Check distance to player
        if (birdPos.Distance(playerPos) < ai.wakeUpRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] detected player at distance: " << birdPos.Distance(playerPos) << std::endl;
            ai.state = BirdState::WAKING;
            ai.stateTimer = ai.wakeUpTime;
            // TODO: Trigger "Waking" animation here
        }
    }

    void AISystem::HandleWakingState(Entity bird, BirdAIComponent& ai) {

        EntityManager entityMgr(*ecs_);
        // Stop moving during wake-up animation
        if (entityMgr.HasComponent(bird, VelocityType)) {
            entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
        }

        ai.stateTimer -= Framework::Time::DeltaTime();
        if (ai.stateTimer <= 0.0f) {
            ai.state = BirdState::CHASE;
            ai.pathRecalculateTimer = 0.0f; // Force immediate pathfinding
        }
    }

    void AISystem::HandleChaseState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D birdHalfSize, Vector2D playerPos, Vector2D playerHalfSize) {
        // TODO: Trigger Flying animation here

        EntityManager entityMgr(*ecs_);

        float dist = birdPos.Distance(playerPos);

        // Check if player escaped too far
        if (dist > ai.chaseRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] lost player, going back to IDLE" << std::endl;
            ai.state = BirdState::IDLE;
            ai.currentPath.clear();
            if (entityMgr.HasComponent(bird, VelocityType)) {
                entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
            }
            return;
        }


        // Check if close enough to attack (CRITICAL: Use proper radius)
        if (dist < ai.attackRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] entering ATTACK state!" << std::endl;
            ai.state = BirdState::ATTACK;
            ai.attackTargetPosition = playerPos; // Snapshot position
            ai.stateTimer = ai.attackDuration;
            ai.currentPath.clear(); // Clear pathfinding
            return;
        }

        // Continue chasing with A*
        ai.pathRecalculateTimer -= Framework::Time::DeltaTime();
        if (ai.pathRecalculateTimer <= 0.0f || ai.currentPath.empty()) {
            ai.currentPath = pather_.FindPath(navGrid_, birdPos, playerPos);
            ai.pathRecalculateTimer = ai.pathRecalculateInterval;
        }

        // Follow the path
        FollowPath(bird, ai, birdPos);
    }

    void AISystem::HandleHoverState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D birdHalfSize, Vector2D playerPos, Vector2D playerHalfSize) {
        // TODO: Trigger Flying animation here

        EntityManager entityMgr(*ecs_);

        float distToPlayer = birdPos.Distance(playerPos);

        // Check if player is in attack range again
        if (distToPlayer < ai.attackRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] re-engaging from hover!" << std::endl;
            ai.state = BirdState::ATTACK;
            ai.attackTargetPosition = playerPos;
            ai.stateTimer = ai.attackDuration;
            return;
        }

        // Check if player moved too far away
        if (distToPlayer > ai.hoverRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] resuming chase from hover" << std::endl;
            ai.state = BirdState::CHASE;
            ai.pathRecalculateTimer = 0.0f;
            return;
        }

        // Simple hover behavior: orbit above player
        Vector2D targetPos = playerPos + Vector2D(0.0f, ai.hoverHeight);
        Vector2D direction = (targetPos - birdPos).Normalized();

        if (entityMgr.HasComponent(bird, VelocityType)) {
            entityMgr.GetComponent<Velocity>(bird).Vel = direction * ai.hoverSpeed;
        }
    }

    void AISystem::HandleAttackState(Entity bird, BirdAIComponent& ai) {
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(bird, PositionType) ||
            !entityMgr.HasComponent(bird, VelocityType)) {
            return;
        }

        Vector2D birdPos = entityMgr.GetComponent<Position>(bird).Pos;

        // Dive straight at snapshot position
        Vector2D direction = (ai.attackTargetPosition - birdPos).Normalized();
        entityMgr.GetComponent<Velocity>(bird).Vel = direction * ai.attackSpeed;

        ai.stateTimer -= Framework::Time::DeltaTime();

        // Check collision with platforms or timeout
        // (Physics will handle player collision separately)
        bool hitWall = false;

        // Simple wall detection: if velocity suddenly drops to near-zero
        Vector2D currentVel = entityMgr.GetComponent<Velocity>(bird).Vel;
        if (currentVel.Magnitude() < ai.attackSpeed * 0.5f) {
            hitWall = true;
        }

        if (hitWall || ai.stateTimer <= 0.0f) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] attack ended, entering STUNNED state" << std::endl;
            ai.state = BirdState::STUNNED;
            ai.stateTimer = ai.stunDuration;
            entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
        }
    }

    void AISystem::HandleStunnedState(Entity bird, BirdAIComponent& ai) {
        EntityManager entityMgr(*ecs_);

        // Stay still
        if (entityMgr.HasComponent(bird, VelocityType)) {
            entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
        }

        ai.stateTimer -= Framework::Time::DeltaTime();
        if (ai.stateTimer <= 0.0f) {
            ai.state = BirdState::HOVER;
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] recovering, entering HOVER" << std::endl;
        }
    }

    void AISystem::FollowPath(Entity bird, BirdAIComponent& ai, Vector2D birdPos) {
        EntityManager entityMgr(*ecs_);
        if (ai.currentPath.empty()) {
            // No path, just move towards player (dumb move)
            Entity player = GetPlayer();
            if (player == 0) return;

            if (!entityMgr.HasComponent(player, PositionType)) {
                Logger::Get().Error(DebugType::AISystem) << "Player is missing Position component!" << std::endl;
                return;
            }
            if (!entityMgr.HasComponent(bird, VelocityType)) {
                Logger::Get().Error(DebugType::AISystem) << "Bird Enemy is missing Velocity component!" << std::endl;
                return;
            }

            Vector2D dir = (entityMgr.GetComponent<Position>(player).Pos - birdPos).Normalized();
            entityMgr.GetComponent<Velocity>(bird).Vel = dir * ai.moveSpeed;
            return;
        }

        // Get the next waypoint
        Vector2D targetPoint = ai.currentPath[0];

        // Move towards waypoint
        Vector2D direction = (targetPoint - birdPos).Normalized();
        entityMgr.GetComponent<Velocity>(bird).Vel = direction * ai.moveSpeed;

        // Check if we reached the waypoint
        if (birdPos.Distance(targetPoint) < navGrid_.cellSize) {

            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] reached waypoint. "
                << (ai.currentPath.size() - 1) << " remaining" << std::endl;

            // Arrived at waypoint, remove it from path
            ai.currentPath.erase(ai.currentPath.begin());
        }
    }

    // ============================================================================
    // SKELETON AI HANDLERS
    // ============================================================================

    void AISystem::HandleSkeletonIdleState(Entity skeleton, SkeletonAIComponent& ai, Vector2D skeletonPos, Vector2D playerPos) {
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(skeleton, VelocityType)) {
            Logger::Get().Error(DebugType::AISystem) << "Skeleton Enemy is missing Velocity component!" << std::endl;
            return;
        }

        // Stop moving
        entityMgr.GetComponent<Velocity>(skeleton).Vel.x = 0.0f;
        // TODO: Trigger Idle animation here

        // Check distance to player
        if (skeletonPos.Distance(playerPos) < ai.detectionRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Skeleton [" << skeleton << "] detected player." << std::endl;
            ai.state = SkeletonState::CHASE;
        }
    }

    void AISystem::HandleSkeletonChaseState(Entity skeleton, SkeletonAIComponent& ai, Vector2D skeletonPos, Vector2D playerPos) {
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(skeleton, PlayerStateType)) {
            Logger::Get().Error(DebugType::AISystem) << "Skeleton Enemy is missing PlayerState component!" << std::endl;
            return;
        }
        if (!entityMgr.HasComponent(skeleton, VelocityType)) {
            Logger::Get().Error(DebugType::AISystem) << "Skeleton Enemy is missing Velocity component!" << std::endl;
            return;
        }

        // First, check if player has gone out of range
        if (skeletonPos.Distance(playerPos) > ai.detectionRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Skeleton [" << skeleton << "] lost player." << std::endl;
            ai.state = SkeletonState::IDLE;
            return; // Exit, HandleSkeletonIdleState will stop movement next frame
        }

        // Player is in range, move towards them
        // This AI only walks on platforms, so only control X-vel

        // PlayerState to check if grounded and to set our facing direction
        PlayerState& state = entityMgr.GetComponent<PlayerState>(skeleton);

        // Only move if on the ground
        if (state.isGrounded) {
            auto& vel = entityMgr.GetComponent<Velocity>(skeleton).Vel;

            if (playerPos.x < skeletonPos.x - 0.01f) { // Move left 
                vel.x = -ai.moveSpeed;
                state.isFacingRight = false;
                // TODO: Trigger "Walk" animation
            }
            else if (playerPos.x > skeletonPos.x + 0.01f) { // Move right 
                vel.x = ai.moveSpeed;
                state.isFacingRight = true;
                // TODO: Trigger Walk animation
            }
            else {
                // Player is roughly aligned vertically
                vel.x = 0.0f;
                // TODO: Trigger Idle animation
            }
        }
        // (If not grounded, do nothing and let gravity take over)
    }


    // ============================================================================
    // DEBUG DRAWING - ONLY COMPILED IN EDITOR BUILD
    // ============================================================================
#ifdef BUILD_EDITOR

    // Helper to convert our Color struct to ImGui's IM_COL32
    inline ImU32 ToImColor(const Color& color) {
        return IM_COL32(
            static_cast<int>(color.r * 255),
            static_cast<int>(color.g * 255),
            static_cast<int>(color.b * 255),
            static_cast<int>(color.a * 255)
        );
    }


    // Add this to the top of AISystem::DrawDebug() for debugging

    void AISystem::DrawDebug(CameraSystem* cameraSystem, ImDrawList* drawList, Editor::Editor* editor, Entity cameraToUse) {
        // *** ADD COMPREHENSIVE CHECKS ***
        if (!debugVisualization) {
            Logger::Get().Error(DebugType::AISystem) << "Debug visualization is disabled" << std::endl;
            return;
        }

        if (!gfx_) {
            Logger::Get().Error(DebugType::AISystem) << "Graphics system is null!" << std::endl;
            return;
        }

        if (!cameraSystem) {
            Logger::Get().Error(DebugType::AISystem) << "Camera system is null!" << std::endl;
            return;
        }

        if (!drawList) {
            Logger::Get().Error(DebugType::AISystem) << "DrawList is null!" << std::endl;
            return;
        }

        if (!editor) {
            Logger::Get().Error(DebugType::AISystem) << "Editor is null!" << std::endl;
            return;
        }

        EntityManager entityMgr(*ecs_);
        Entity player = GetPlayer();

        if (player == 0) {
            Logger::Get().Error(DebugType::AISystem) << "No player found for debug drawing" << std::endl;
            return;
        }
        if (!entityMgr.HasComponent(player, PositionType)) {
            Logger::Get().Error(DebugType::AISystem) << "Player is missing Position component for debug drawing" << std::endl;
            return;
        }

        Vector2D playerPos = entityMgr.GetComponent<Position>(player).Pos;
        Entity activeCam = cameraToUse; 

        if (activeCam == 0) {
            Logger::Get().Error(DebugType::AISystem) << "No camera was provided for debug draw!" << std::endl;
            return;
        }

        // Get view bounds
        float viewLeft, viewRight, viewBottom, viewTop;
        cameraSystem->GetViewBounds(activeCam, viewLeft, viewRight, viewBottom, viewTop);

        static bool gridTestLogged = false;
        if (!gridTestLogged) {
            // Get screen coords for a single 1x1 world-space square at origin
            float halfCell = navGrid_.cellSize * 0.5f;
            ImVec2 screenMin = editor->WorldToImGuiScreen({ -halfCell, -halfCell });
            ImVec2 screenMax = editor->WorldToImGuiScreen({ halfCell,  halfCell });

            // Calculate the width and height in PIXELS
            // screenMin.y will be > screenMax.y because screen-space Y is inverted
            float screenWidth = std::abs(screenMax.x - screenMin.x);
            float screenHeight = std::abs(screenMax.y - screenMin.y);

            Logger::Get().Info(DebugType::AISystem)
                << "Grid Symmetry Test: Cell Size = " << navGrid_.cellSize << " world units"
                << " | Rendered as: " << screenWidth << "px (Width) x "
                << screenHeight << "px (Height)" << std::endl;

            if (std::abs(screenWidth - screenHeight) > 1.0f) {
                Logger::Get().Error(DebugType::AISystem)
                    << "GRID IS NOT SQUARE! Camera/Viewport aspect ratio mismatch." << std::endl;
            }
            else {
                Logger::Get().Info(DebugType::AISystem) << "GRID IS SQUARE." << std::endl;
            }
            gridTestLogged = true;
        }
        // --- END TEST ---

        // Draw navigation grid
        if (showNavigationGrid) {
            DrawNavigationGrid(viewLeft, viewRight, viewBottom, viewTop, drawList, editor, activeCam);
            Logger::Get().Info(DebugType::AISystem) << "Drawing navigation grid" << std::endl;
        }

        //Draw Enemies debug info
        std::vector<Entity> Enemies = entityMgr.GetAllEntitiesWithComponent(EnemyTagType); 
       

        for (const Entity& bird : Enemies) {
            if (!entityMgr.HasComponent(bird, BirdAIComponentType) ||
                !entityMgr.HasComponent(bird, PositionType))
            {
                Logger::Get().Error(DebugType::AISystem) << "Bird Enemy is missing BirdAIComponent/Position components!" << std::endl;
                continue;
            }

            Framework::BirdAIComponent& ai = entityMgr.GetComponent<BirdAIComponent>(bird);
            Vector2D birdPos = entityMgr.GetComponent<Position>(bird).Pos;


            if (showBirdDebug) {
                DrawBirdDebug(birdPos, ai, playerPos, drawList, editor, activeCam); 
            }

            if (showPaths && !ai.currentPath.empty()) {
                Color pathColor = { 1.0f, 0.0f, 1.0f, 0.8f };
                DrawPath(ai.currentPath, pathColor, drawList, editor, activeCam);
            }

            if (showColliders && entityMgr.HasComponent(bird, AABBColliderType)) {
                Vector2D halfSize = entityMgr.GetComponent<AABBCollider>(bird).HalfSize;
                DrawAABB(birdPos, halfSize, { 0.0f, 1.0f, 0.0f, 0.5f }, drawList, editor, activeCam); 
            }
        }
    }

    void AISystem::DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) { 
        Vector2D gridMin = navGrid_.WorldToGrid({ viewLeft, viewBottom });
        Vector2D gridMax = navGrid_.WorldToGrid({ viewRight, viewTop });

        Color walkableColor = { 0.0f, 1.0f, 0.0f, 0.2f };
        Color blockedColor = { 1.0f, 0.0f, 0.0f, 0.2f };
        ImU32 imWalkable = ToImColor(walkableColor);
        ImU32 imBlocked = ToImColor(blockedColor);

        for (int x = static_cast<int>(gridMin.x) - 1; x <= static_cast<int>(gridMax.x) + 1; ++x) {
            for (int y = static_cast<int>(gridMin.y) - 1; y <= static_cast<int>(gridMax.y) + 1; ++y) {
                Vector2D worldPos = navGrid_.GridToWorld(x, y);
                Vector2D min = { worldPos.x - navGrid_.cellSize * 0.5f, worldPos.y - navGrid_.cellSize * 0.5f };
                Vector2D max = { worldPos.x + navGrid_.cellSize * 0.5f, worldPos.y + navGrid_.cellSize * 0.5f };

                ImVec2 screenMin = editor->WorldToImGuiScreen(min, cameraID); 
                ImVec2 screenMax = editor->WorldToImGuiScreen(max, cameraID); 
                ImVec2 topLeft = { std::min(screenMin.x, screenMax.x), std::min(screenMin.y, screenMax.y) };
                ImVec2 botRight = { std::max(screenMin.x, screenMax.x), std::max(screenMin.y, screenMax.y) };

                drawList->AddRectFilled(topLeft, botRight, navGrid_.IsWalkable(x, y) ? imWalkable : imBlocked);
            }
        }
    }

    void AISystem::DrawBirdDebug(const Vector2D& birdPos, const BirdAIComponent& ai, const Vector2D& playerPos, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) { 
        // State-based colors
        Color stateColor = { 0.5f, 0.5f, 0.5f, 0.8f }; // IDLE
        switch (ai.state) {
        case BirdState::WAKING: stateColor = { 1.0f, 1.0f, 0.0f, 0.8f }; break; // Yellow
        case BirdState::CHASE:  stateColor = { 1.0f, 0.5f, 0.0f, 0.8f }; break; // Orange
        case BirdState::HOVER:  stateColor = { 0.0f, 1.0f, 1.0f, 0.8f }; break; // Cyan
        case BirdState::ATTACK: stateColor = { 1.0f, 0.0f, 0.0f, 0.8f }; break; // Red
        }

        // Draw a rectangle at the bird's feet indicating state
        Vector2D halfSize = { 0.1f, 0.1f };
        Vector2D min = birdPos - halfSize;
        Vector2D max = birdPos + halfSize;
        ImVec2 screenMin = editor->WorldToImGuiScreen(min, cameraID); 
        ImVec2 screenMax = editor->WorldToImGuiScreen(max, cameraID); 
        ImVec2 topLeft = { std::min(screenMin.x, screenMax.x), std::min(screenMin.y, screenMax.y) };
        ImVec2 botRight = { std::max(screenMin.x, screenMax.x), std::max(screenMin.y, screenMax.y) };
        drawList->AddRectFilled(topLeft, botRight, ToImColor(stateColor));

        // Radii
        DrawCircle(birdPos, ai.wakeUpRadius, { 0.7f, 0.7f, 0.0f, 0.5f }, drawList, editor, cameraID); 
        DrawCircle(birdPos, ai.hoverRadius, { 0.0f, 0.7f, 0.7f, 0.5f }, drawList, editor, cameraID);
        DrawCircle(birdPos, ai.attackRadius, { 1.0f, 0.0f, 0.0f, 0.5f }, drawList, editor, cameraID); 

        // Line to player
        if (ai.state != BirdState::IDLE) {
            drawList->AddLine(editor->WorldToImGuiScreen(birdPos, cameraID), editor->WorldToImGuiScreen(playerPos, cameraID), ToImColor({ 1.0f, 1.0f, 1.0f, 0.3f })); 
        }

        // Attack target
        if (ai.state == BirdState::ATTACK) {
            DrawArrow(birdPos, ai.attackTargetPosition, { 1.0f, 0.0f, 0.0f, 1.0f }, drawList, editor, cameraID); 
        }

        if (ai.state == BirdState::HOVER) {
            float boxHalfWidth = ai.hoverRadius;
            float boxHalfHeight = ai.hoverRadius;

            Vector2D nodeA = { playerPos.x - boxHalfWidth, playerPos.y + ai.hoverHeight };
            Vector2D nodeB = { playerPos.x + boxHalfWidth, playerPos.y + ai.hoverHeight };
            Vector2D nodeC = { playerPos.x + boxHalfWidth, playerPos.y + ai.hoverHeight + boxHalfHeight };
            Vector2D nodeD = { playerPos.x - boxHalfWidth, playerPos.y + ai.hoverHeight + boxHalfHeight };

            ImU32 boxColor = ToImColor({ 0.0f, 1.0f, 1.0f, 0.3f }); // Cyan, semi-transparent

            // Draw the box outline
            drawList->AddLine(editor->WorldToImGuiScreen(nodeA, cameraID), editor->WorldToImGuiScreen(nodeB, cameraID), boxColor); // MODIFIED
            drawList->AddLine(editor->WorldToImGuiScreen(nodeB, cameraID), editor->WorldToImGuiScreen(nodeC, cameraID), boxColor); // MODIFIED
            drawList->AddLine(editor->WorldToImGuiScreen(nodeC, cameraID), editor->WorldToImGuiScreen(nodeD, cameraID), boxColor); // MODIFIED
            drawList->AddLine(editor->WorldToImGuiScreen(nodeD, cameraID), editor->WorldToImGuiScreen(nodeA, cameraID), boxColor); // MODIFIED
        }
    }

    void AISystem::DrawPath(const std::vector<Vector2D>& path, const Color& color, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        if (path.size() < 2) return;

        ImU32 imLineColor = ToImColor(color);
        ImU32 imWaypointColor = ToImColor({ color.r, color.g, color.b, 0.5f });
        Vector2D waypointHalfSize = { 0.02f, 0.02f };

        for (size_t i = 0; i < path.size() - 1; ++i) {
            drawList->AddLine(editor->WorldToImGuiScreen(path[i], cameraID), editor->WorldToImGuiScreen(path[i + 1], cameraID), imLineColor);

            // Draw waypoint marker
            ImVec2 screenMin = editor->WorldToImGuiScreen(path[i] - waypointHalfSize, cameraID);
            ImVec2 screenMax = editor->WorldToImGuiScreen(path[i] + waypointHalfSize, cameraID);
            ImVec2 topLeft = { std::min(screenMin.x, screenMax.x), std::min(screenMin.y, screenMax.y) };
            ImVec2 botRight = { std::max(screenMin.x, screenMax.x), std::max(screenMin.y, screenMax.y) };
            drawList->AddRectFilled(topLeft, botRight, imWaypointColor);
        }
        // Draw last waypoint
        ImVec2 screenMin = editor->WorldToImGuiScreen(path.back() - waypointHalfSize, cameraID);
        ImVec2 screenMax = editor->WorldToImGuiScreen(path.back() + waypointHalfSize, cameraID);
        ImVec2 topLeft = { std::min(screenMin.x, screenMax.x), std::min(screenMin.y, screenMax.y) };
        ImVec2 botRight = { std::max(screenMin.x, screenMax.x), std::max(screenMin.y, screenMax.y) };
        drawList->AddRectFilled(topLeft, botRight, imWaypointColor);
    }

    void AISystem::DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        Vector2D min = center - halfSize;
        Vector2D max = center + halfSize;

        ImVec2 screenMin = editor->WorldToImGuiScreen(min, cameraID);
        ImVec2 screenMax = editor->WorldToImGuiScreen(max, cameraID);

        ImVec2 topLeft = { std::min(screenMin.x, screenMax.x), std::min(screenMin.y, screenMax.y) };
        ImVec2 botRight = { std::max(screenMin.x, screenMax.x), std::max(screenMin.y, screenMax.y) };

        drawList->AddRect(topLeft, botRight, ToImColor(color));
    }

    void AISystem::DrawCircle(const Vector2D& center, float radius, const Color& color, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID, int segments) { // MODIFIED
        // We draw with lines to account for non-uniform camera scaling (aspect ratio)
        Vector2D worldPrevPoint = center + Vector2D(radius, 0);
        ImVec2 prevPoint = editor->WorldToImGuiScreen(worldPrevPoint, cameraID); 
        ImU32 imColor = ToImColor(color);

        for (int i = 1; i <= segments; ++i) {
            float angle = (2.0f * 3.14159265f / segments) * i;
            Vector2D nextWorldPoint = center + Vector2D(std::cos(angle) * radius, std::sin(angle) * radius);
            ImVec2 nextPoint = editor->WorldToImGuiScreen(nextWorldPoint, cameraID); 
            drawList->AddLine(prevPoint, nextPoint, imColor);
            prevPoint = nextPoint;
        }
    }

    void AISystem::DrawArrow(const Vector2D& start, const Vector2D& end, const Color& color, ImDrawList* drawList, Editor::Editor* editor, Entity cameraID) {
        ImU32 imColor = ToImColor(color);
        ImVec2 screenStart = editor->WorldToImGuiScreen(start, cameraID);
        ImVec2 screenEnd = editor->WorldToImGuiScreen(end, cameraID);

        drawList->AddLine(screenStart, screenEnd, imColor);

        Vector2D dir = (end - start).Normalized();
        Vector2D perp = { -dir.y, dir.x }; // Perpendicular
        float arrowheadSize = 0.1f;

        // Calculate arrowhead points in world space
        Vector2D p1_world = end - dir * arrowheadSize + perp * arrowheadSize * 0.5f;
        Vector2D p2_world = end - dir * arrowheadSize - perp * arrowheadSize * 0.5f;

        // Convert to screen space and draw
        drawList->AddLine(screenEnd, editor->WorldToImGuiScreen(p1_world, cameraID), imColor);
        drawList->AddLine(screenEnd, editor->WorldToImGuiScreen(p2_world, cameraID), imColor);
    }

#endif // BUILD_EDITOR

} // namespace Framework

