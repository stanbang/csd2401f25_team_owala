#include "AISystem.h"
#include "EntityManager.h" // Assuming you have this from your other files
#include "EntityStorage.h" // For ComponentType enums
#include <cmath>           // For std::abs and std::sin
#include <cstdlib>         // For rand()
#include <algorithm>       // For std::clamp
#include "Logger.h"         // NEW: Include Logger
#include "CameraSystem.h"   // NEW: Include CameraSystem

namespace Framework {

    AISystem::AISystem(EntityStorage& storage, Graphics& gfx)
        : ecs_(&storage), gfx_(&gfx) {}

    void AISystem::Initialize() {
        // Build the navigation grid for the first time
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

    void AISystem::Update(float dt) {
        EntityManager entityMgr(*ecs_);

        // Find the player
        Entity player = GetPlayer();
        if (player == 0) {
            // NEW: Add Log
            static bool playerWarningLogged = false;
            if (!playerWarningLogged) {
                Logger::Get().Info(DebugType::AISystem) << "No player found in scene!" << std::endl;
                playerWarningLogged = true;
            }
            return;
        }

        Vector2D playerPos = entityMgr.GetComponent<Position>(player).Pos;

        // Rebuild grid periodically in case platforms move
        gridRebuildTimer_ += dt;
        if (gridRebuildTimer_ >= gridRebuildInterval_) {
            navGrid_.BuildGrid(*ecs_);
            gridRebuildTimer_ = 0.0f;

            Logger::Get().Info(DebugType::AISystem) << "Navigation grid rebuilt." << std::endl;
        }

        // Loop over all entities with a BirdAITag and BirdAIComponent
        for (const Entity& bird : entityMgr.GetAllEntitiesWithComponent(BirdAITagType)) {
            // Check that the bird has all components before proceeding
            if (!entityMgr.HasComponent(bird, BirdAIComponentType) ||
                !entityMgr.HasComponent(bird, PositionType) ||
                !entityMgr.HasComponent(bird, VelocityType))
            {
                continue; // Skip if bird doesn't have all required components
            }

            auto& ai = entityMgr.GetComponent<BirdAIComponent>(bird);
            auto& birdPos = entityMgr.GetComponent<Position>(bird).Pos;

            // --- NEW: Log state transitions ---
            // We use a map to track each bird's last state
            static std::map<Entity, BirdState> lastStates;
            if (lastStates.find(bird) == lastStates.end()) {
                lastStates[bird] = ai.state;
            }

            if (ai.state != lastStates[bird]) {
                std::string stateName;
                switch (ai.state) {
                case BirdState::IDLE:   stateName = "IDLE";   break;
                case BirdState::WAKING: stateName = "WAKING"; break;
                case BirdState::CHASE:  stateName = "CHASE";  break;
                case BirdState::HOVER:  stateName = "HOVER";  break;
                case BirdState::ATTACK: stateName = "ATTACK"; break;
                }
                Logger::Get().Info(DebugType::AISystem)
                    << "Bird [" << bird << "] State Change: " << stateName << std::endl;
                lastStates[bird] = ai.state;
            }
            // --- End new log block ---

            // --- Main State Machine ---
            switch (ai.state) {
            case BirdState::IDLE:
                HandleIdleState(bird, ai, birdPos, playerPos);
                break;
            case BirdState::WAKING:
                HandleWakingState(bird, ai, dt);
                break;
            case BirdState::CHASE:
                HandleChaseState(bird, ai, birdPos, playerPos, dt);
                break;
            case BirdState::HOVER:
                HandleHoverState(bird, ai, birdPos, playerPos, dt);
                break;
            case BirdState::ATTACK:
                HandleAttackState(bird, ai, dt);
                break;
            }
        }
    }

    void AISystem::HandleIdleState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos) {
        EntityManager entityMgr(*ecs_);
        entityMgr.GetComponent<Velocity>(bird).Vel = Vector2D::Zero();
        // TODO: Trigger "Sleeping" animation here
        // (e.g., if (entityMgr.HasComponent(bird, AnimationComponentType)) { ... } )

        // Check distance to player
        if (birdPos.Distance(playerPos) < ai.wakeUpRadius) {
            // NEW: Add Log
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] detected player at distance: " << birdPos.Distance(playerPos) << std::endl;
            ai.state = BirdState::WAKING;
            ai.stateTimer = ai.wakeUpTime;
            // TODO: Trigger "Waking" animation here
        }
    }

    void AISystem::HandleWakingState(Entity bird, BirdAIComponent& ai, float dt) {
        ai.stateTimer -= dt;
        if (ai.stateTimer <= 0.0f) {
            Logger::Get().Info(DebugType::AISystem) << "Bird [" << bird << "] finished waking up!" << std::endl;
            ai.state = BirdState::CHASE;
            ai.pathRecalculateTimer = 0.0f; // Force path recalculation immediately
        }
    }

    void AISystem::HandleChaseState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos, float dt) {
        // TODO: Trigger "Flying" animation here

        float dist = birdPos.Distance(playerPos);

        // 1. Check if we've arrived
        if (dist < ai.hoverRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] reached hover radius!" << std::endl;
            ai.state = BirdState::HOVER;
            // Set a random attack timer for when we start hovering
            ai.attackTimer = (static_cast<float>(rand() % 100) / 100.0f) * ai.attackCooldown + 1.0f; // 1-4 sec
            ai.currentPath.clear();
            return;
        }

        // 2. Recalculate path if needed
        ai.pathRecalculateTimer -= dt;
        if (ai.pathRecalculateTimer <= 0.0f || ai.currentPath.empty()) {

            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] calculating path from ("
                << birdPos.x << ", " << birdPos.y << ") to ("
                << playerPos.x << ", " << playerPos.y << ")" << std::endl;

            ai.currentPath = pather_.FindPath(navGrid_, birdPos, playerPos);
            ai.pathRecalculateTimer = ai.pathRecalculateInterval;

            // NEW: Add Log
            if (ai.currentPath.empty()) {
                Logger::Get().Info(DebugType::AISystem)
                    << "Bird [" << bird << "] failed to find path!" << std::endl;
            }
            else {
                Logger::Get().Info(DebugType::AISystem)
                    << "Bird [" << bird << "] found path with " << ai.currentPath.size() << " waypoints" << std::endl;
            }
        }

        // 3. Follow the path
        FollowPath(bird, ai, birdPos, dt);
    }

    void AISystem::HandleHoverState(Entity bird, BirdAIComponent& ai, Vector2D birdPos, Vector2D playerPos, float dt) {
        // TODO: Trigger "Flying" animation here

        EntityManager entityMgr(*ecs_);

        // Check if player moved too far away
        float distToPlayer = birdPos.Distance(playerPos);
        if (distToPlayer > ai.hoverRadius * 2.0f) { // Increased escape threshold
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] player escaped, resuming chase!" << std::endl;
            ai.state = BirdState::CHASE;
            ai.pathRecalculateTimer = 0.0f;
            return;
        }

        // Calculate the 5 hover nodes in a circle above the player
        Vector2D hoverNodes[5];
        for (int i = 0; i < 5; ++i) {
            float angle = (2.0f * 3.14159265f / 5.0f) * i; // Divide circle into 5 equal parts
            hoverNodes[i] = {
                playerPos.x + ai.hoverRadius_Circle * std::cos(angle),
                playerPos.y + ai.hoverHeight + ai.hoverRadius_Circle * std::sin(angle)
            };
        }

        // Get the current target node
        Vector2D targetNode = hoverNodes[ai.currentHoverNode];

        // Move towards the current node
        Vector2D direction = (targetNode - birdPos).Normalized();
        entityMgr.GetComponent<Velocity>(bird).Vel = direction * ai.hoverSpeed;

        // Check if we've reached the current node
        if (birdPos.Distance(targetNode) < ai.hoverNodeReachedDist) {
            // Move to next node
            ai.currentHoverNode = (ai.currentHoverNode + 1) % 5;
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] reached hover node, moving to node " << ai.currentHoverNode << std::endl;
        }

        // Check for attack opportunity
        ai.attackTimer -= dt;
        if (ai.attackTimer <= 0.0f && distToPlayer < ai.attackRadius) {
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] initiating attack!" << std::endl;
            ai.state = BirdState::ATTACK;
            ai.attackTargetPosition = playerPos; // Lock player position
            ai.stateTimer = ai.attackDuration;
        }
    }

    void AISystem::HandleAttackState(Entity bird, BirdAIComponent& ai, float dt) {
        ai.stateTimer -= dt;
        EntityManager entityMgr(*ecs_);
        Vector2D birdPos = entityMgr.GetComponent<Position>(bird).Pos;

        // 1. Dive at target
        Vector2D direction = (ai.attackTargetPosition - birdPos).Normalized();
        entityMgr.GetComponent<Velocity>(bird).Vel = direction * ai.attackSpeed;

        // 2. Check if attack finished (timer up OR close to target)
        if (ai.stateTimer <= 0.0f || birdPos.Distance(ai.attackTargetPosition) < 0.5f) {

            // NEW: Add Log
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] attack finished!" << std::endl;

            // Finished attack, reset attack cooldown
            ai.attackTimer = (static_cast<float>(rand() % 100) / 100.0f) * ai.attackCooldown + 1.0f;

            // Go back to CHASE or HOVER
            Entity player = GetPlayer();
            if (player == 0) {
                ai.state = BirdState::IDLE; // Player is gone?
                return;
            }

            float dist = birdPos.Distance(entityMgr.GetComponent<Position>(player).Pos);
            if (dist > ai.hoverRadius) {
                ai.state = BirdState::CHASE;
            }
            else {
                ai.state = BirdState::HOVER;
            }
        }
    }

    void AISystem::FollowPath(Entity bird, BirdAIComponent& ai, Vector2D birdPos, float dt) {
        EntityManager entityMgr(*ecs_);
        if (ai.currentPath.empty()) {
            // No path, just move towards player (dumb move)
            Entity player = GetPlayer();
            if (player == 0) return;

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

            // NEW: Add Log
            Logger::Get().Info(DebugType::AISystem)
                << "Bird [" << bird << "] reached waypoint. "
                << (ai.currentPath.size() - 1) << " remaining" << std::endl;

            // Arrived at waypoint, remove it from path
            ai.currentPath.erase(ai.currentPath.begin());
        }
    }

    // --- NEW: IMPLEMENT ALL DEBUG DRAW FUNCTIONS ---

    void AISystem::DrawHoverNodes(const Vector2D& playerPos, const BirdAIComponent& ai) {
        // Draw the 5 hover nodes
        for (int i = 0; i < 5; ++i) {
            float angle = (2.0f * 3.14159265f / 5.0f) * i;
            Vector2D nodePos = {
                playerPos.x + ai.hoverRadius_Circle * std::cos(angle),
                playerPos.y + ai.hoverHeight + ai.hoverRadius_Circle * std::sin(angle)
            };

            // Highlight the current target node
            Color nodeColor = (i == ai.currentHoverNode)
                ? Color{ 1.0f, 1.0f, 0.0f, 1.0f }  // Yellow for current target
            : Color{ 0.0f, 1.0f, 1.0f, 0.6f }; // Cyan for others

            // Draw a small square for each node
            Vector2D halfSize = { 0.05f, 0.05f };
            gfx_->drawRectangle(nodePos - halfSize, nodePos + halfSize, nodeColor);

            // Draw lines connecting the nodes in a circle
            int nextNode = (i + 1) % 5;
            float nextAngle = (2.0f * 3.14159265f / 5.0f) * nextNode;
            Vector2D nextNodePos = {
                playerPos.x + ai.hoverRadius_Circle * std::cos(nextAngle),
                playerPos.y + ai.hoverHeight + ai.hoverRadius_Circle * std::sin(nextAngle)
            };
            gfx_->drawLine(nodePos, nextNodePos, { 0.0f, 1.0f, 1.0f, 0.3f });
        }
    }

    void AISystem::DrawDebug(CameraSystem* cameraSystem) {
        if (!debugVisualization || !gfx_ || !cameraSystem) return;

        EntityManager entityMgr(*ecs_);
        Entity player = GetPlayer();
        if (player == 0) return;

        Vector2D playerPos = entityMgr.GetComponent<Position>(player).Pos;
        Entity activeCam = cameraSystem->GetActiveCamera();
        if (activeCam == 0) return;

        // Get view bounds from the camera system
        float viewLeft, viewRight, viewBottom, viewTop;
        cameraSystem->GetViewBounds(activeCam, viewLeft, viewRight, viewBottom, viewTop);

        // 1. Draw navigation grid
        if (showNavigationGrid) {
            DrawNavigationGrid(viewLeft, viewRight, viewBottom, viewTop);
        }

        // 2. Draw debug info for each bird
        for (const Entity& bird : entityMgr.GetAllEntitiesWithComponent(BirdAITagType)) {
            if (!entityMgr.HasComponent(bird, BirdAIComponentType) ||
                !entityMgr.HasComponent(bird, PositionType))
            {
                continue;
            }

            auto& ai = entityMgr.GetComponent<BirdAIComponent>(bird);
            Vector2D birdPos = entityMgr.GetComponent<Position>(bird).Pos;

            // Draw bird-specific debug info (radii, state, etc.)
            if (showBirdDebug) {
                DrawBirdDebug(birdPos, ai, playerPos);
            }

            // Draw current path
            if (showPaths && !ai.currentPath.empty()) {
                Color pathColor = { 1.0f, 0.0f, 1.0f, 0.8f }; // Magenta
                DrawPath(ai.currentPath, pathColor);
            }

            // Draw collider
            if (showColliders && entityMgr.HasComponent(bird, AABBColliderType)) {
                Vector2D halfSize = entityMgr.GetComponent<AABBCollider>(bird).HalfSize;
                DrawAABB(birdPos, halfSize, { 0.0f, 1.0f, 0.0f, 0.5f }); // Semi-transparent green
            }
        }
    }

    void AISystem::DrawNavigationGrid(float viewLeft, float viewRight, float viewBottom, float viewTop) {
        // Find grid boundaries to draw
        Vector2D gridMin = navGrid_.WorldToGrid({ viewLeft, viewBottom });
        Vector2D gridMax = navGrid_.WorldToGrid({ viewRight, viewTop });

        Color walkableColor = { 0.0f, 1.0f, 0.0f, 0.2f }; // Semi-transparent green
        Color blockedColor = { 1.0f, 0.0f, 0.0f, 0.2f }; // Semi-transparent red

        for (int x = static_cast<int>(gridMin.x) - 1; x <= static_cast<int>(gridMax.x) + 1; ++x) {
            for (int y = static_cast<int>(gridMin.y) - 1; y <= static_cast<int>(gridMax.y) + 1; ++y) {
                Vector2D worldPos = navGrid_.GridToWorld(x, y);
                // Draw rectangles for cells
                Vector2D min = { worldPos.x - navGrid_.cellSize * 0.5f, worldPos.y - navGrid_.cellSize * 0.5f };
                Vector2D max = { worldPos.x + navGrid_.cellSize * 0.5f, worldPos.y + navGrid_.cellSize * 0.5f };

                if (navGrid_.IsWalkable(x, y)) {
                    gfx_->drawRectangle(min, max, walkableColor);
                }
                else {
                    gfx_->drawRectangle(min, max, blockedColor);
                }
            }
        }
    }

    void AISystem::DrawBirdDebug(const Vector2D& birdPos, const BirdAIComponent& ai, const Vector2D& playerPos) {
        // State-based colors
        Color stateColor = { 0.5f, 0.5f, 0.5f, 0.8f }; // IDLE (Gray)
        switch (ai.state) {
        case BirdState::WAKING: stateColor = { 1.0f, 1.0f, 0.0f, 0.8f }; break; // Yellow
        case BirdState::CHASE:  stateColor = { 1.0f, 0.5f, 0.0f, 0.8f }; break; // Orange
        case BirdState::HOVER:  stateColor = { 0.0f, 1.0f, 1.0f, 0.8f }; break; // Cyan
        case BirdState::ATTACK: stateColor = { 1.0f, 0.0f, 0.0f, 0.8f }; break; // Red
        }
        // Draw a rectangle at the bird's feet indicating state
        Vector2D halfSize = { 0.1f, 0.1f };
        gfx_->drawRectangle(birdPos - halfSize, birdPos + halfSize, stateColor);

        // Radii
        DrawCircle(birdPos, ai.wakeUpRadius, { 0.7f, 0.7f, 0.0f, 0.5f }); // Yellow
        DrawCircle(birdPos, ai.hoverRadius, { 0.0f, 0.7f, 0.7f, 0.5f }); // Cyan
        DrawCircle(birdPos, ai.attackRadius, { 1.0f, 0.0f, 0.0f, 0.5f }); // Red

        // Line to player
        if (ai.state != BirdState::IDLE) {
            gfx_->drawLine(birdPos, playerPos, { 1.0f, 1.0f, 1.0f, 0.3f }); // Faint white line
        }

        // Attack target
        if (ai.state == BirdState::ATTACK) {
            DrawArrow(birdPos, ai.attackTargetPosition, { 1.0f, 0.0f, 0.0f, 1.0f }); // Bright red arrow
        }
    }

    void AISystem::DrawPath(const std::vector<Vector2D>& path, const Color& color) {
        if (path.size() < 2) return;

        Color waypointColor = { color.r, color.g, color.b, 0.5f };
        Vector2D waypointHalfSize = { 0.02f, 0.02f };

        for (size_t i = 0; i < path.size() - 1; ++i) {
            gfx_->drawLine(path[i], path[i + 1], color);
            // Draw waypoint marker
            gfx_->drawRectangle(path[i] - waypointHalfSize, path[i] + waypointHalfSize, waypointColor);
        }
        // Draw last waypoint
        gfx_->drawRectangle(path.back() - waypointHalfSize, path.back() + waypointHalfSize, waypointColor);
    }

    void AISystem::DrawAABB(const Vector2D& center, const Vector2D& halfSize, const Color& color) {
        Vector2D min = center - halfSize;
        Vector2D max = center + halfSize;
        // drawRectangle draws a filled rect, we need an outline
        Vector2D p1 = { min.x, min.y }; // Bottom-left
        Vector2D p2 = { max.x, min.y }; // Bottom-right
        Vector2D p3 = { max.x, max.y }; // Top-right
        Vector2D p4 = { min.x, max.y }; // Top-left
        gfx_->drawLine(p1, p2, color);
        gfx_->drawLine(p2, p3, color);
        gfx_->drawLine(p3, p4, color);
        gfx_->drawLine(p4, p1, color);
    }

    void AISystem::DrawCircle(const Vector2D& center, float radius, const Color& color, int segments) {
        Vector2D prevPoint = center + Vector2D(radius, 0);
        for (int i = 1; i <= segments; ++i) {
            float angle = (2.0f * 3.14159265f / segments) * i;
            Vector2D nextPoint = center + Vector2D(std::cos(angle) * radius, std::sin(angle) * radius);
            gfx_->drawLine(prevPoint, nextPoint, color);
            prevPoint = nextPoint;
        }
    }

    void AISystem::DrawArrow(const Vector2D& start, const Vector2D& end, const Color& color) {
        gfx_->drawLine(start, end, color);
        Vector2D dir = (end - start).Normalized();
        Vector2D perp = { -dir.y, dir.x }; // Perpendicular
        float arrowheadSize = 0.1f;
        // Calculate arrowhead points
        Vector2D p1 = end - dir * arrowheadSize + perp * arrowheadSize * 0.5f;
        Vector2D p2 = end - dir * arrowheadSize - perp * arrowheadSize * 0.5f;
        gfx_->drawLine(end, p1, color);
        gfx_->drawLine(end, p2, color);
    }

} // namespace Framework

