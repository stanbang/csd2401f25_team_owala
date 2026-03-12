/*********************************************************************//**
* \file   Pathfinding.cpp
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
*
* \brief
* Implements the A* pathfinding algorithm, navigation grid generation 
* from colliders, and path-retracing logic.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "Pathfinding.h"
#include "EntityManager.h" 
#include "Components.h"             
#include "GameManager.h"

// for rotation calculation 
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    
    namespace {
       // function object that compares two pathnodes and returns true if bigger
        struct ComparePathNode {
            bool operator()(PathNode* a, PathNode* b) const {
                return a->fCost() > b->fCost();
            }
        };
    }

    // --- NavigationGrid Implementation ---

    //void NavigationGrid::BuildGrid(EntityStorage& ecs) {
    //    wallNodes_.clear();
    //    EntityManager entityMgr(ecs);

    //    // Find all entities that are platforms
    //    for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlatformTagType)) {

    //        // Ensure it has collision and position data
    //        if (entityMgr.HasComponent(entity, AABBColliderType) &&
    //            entityMgr.HasComponent(entity, PositionType)) {

    //            const Vector2D& pos = entityMgr.GetComponent<Position>(entity).Pos;
    //            const Vector2D& halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;

    //            float rotation = 0.0f;
    //            float rotationAngle = 0.0f;
    //            if (entityMgr.HasComponent(entity, RotationFlagType)){
    //                rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
    //            }

    //            if (entityMgr.HasComponent(entity, RotationType)) {
    //                rotationAngle = entityMgr.GetComponent<Rotation>(entity).angle;

    //            }



    //            // Get the AABB of the rotated box
    //            AABB_Bounds worldAABB = PhysicsSystem::GetRotatedAABB(pos, halfSize, rotation);

    //            // add padding to catch cells when rotating 
    //            float padding = cellSize * 0.1f;
    //            Vector2D paddedMin = worldAABB.min - Vector2D(padding, padding);
    //            Vector2D paddedMax = worldAABB.max + Vector2D(padding, padding);
    //            Vector2D gridMin = WorldToGrid(paddedMin);
    //            Vector2D gridMax = WorldToGrid(paddedMax);

    //            // calculate rotation math
    //            float rotationRad = static_cast<float>(rotation * M_PI / 180.0f);
    //            float cos_r = std::cos(rotationRad);
    //            float sin_r = std::sin(rotationRad);

    //            for (int x = static_cast<int>(gridMin.x); x <= static_cast<int>(gridMax.x); ++x) {
    //                for (int y = static_cast<int>(gridMin.y); y <= static_cast<int>(gridMax.y); ++y) {

    //                    Vector2D cellCenterWorld = GridToWorld(x, y);
    //                    Vector2D p_local = cellCenterWorld - pos;

    //                    // Rotate cell center into box's local space
    //                    Vector2D p_rot = { p_local.x * cos_r + p_local.y * sin_r,
    //                                      -p_local.x * sin_r + p_local.y * cos_r };

    //                    // added margin for edge cases where cell might overlap with platform
    //                    float checkMargin = cellSize * 0.1f; // 30% of cell size as margin

    //                    if (p_rot.x >= -(halfSize.x + checkMargin) &&
    //                        p_rot.x <= (halfSize.x + checkMargin) &&
    //                        p_rot.y >= -(halfSize.y + checkMargin) &&
    //                        p_rot.y <= (halfSize.y + checkMargin))
    //                    {
    //                        wallNodes_.insert({ x, y });
    //                    }
    //                }
    //            }
    //        }
    //    }
    //}

    // In Pathfinding.cpp

    //void NavigationGrid::BuildGrid(EntityStorage& ecs) {
    //    wallNodes_.clear();
    //    EntityManager entityMgr(ecs);

    //    // Find all entities that are platforms
    //    for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlatformTagType)) {

    //        // 1. We require Position. We prefer Texture for size, fall back to AABB.
    //        if (!entityMgr.HasComponent(entity, PositionType)) continue;

    //        Vector2D pos = entityMgr.GetComponent<Position>(entity).Pos;
    //        Vector2D baseHalfSize = { 50.0f, 50.0f }; // Default fallback

    //        // PREFER TextureComponent for exact visual matching as requested
    //        if (entityMgr.HasComponent(entity, TextureComponentType)) {
    //            baseHalfSize = entityMgr.GetComponent<TextureComponent>(entity).HalfSize;
    //        }
    //        else if (entityMgr.HasComponent(entity, AABBColliderType)) {
    //            // Note: If TransformationSystem ran, this might already be scaled. 
    //            // Ideally, use TextureComponent for static platforms to be safe.
    //            baseHalfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
    //        }

    //        // 2. Get Scale explicitly to calculate real world size immediately
    //        Vector2D scale = { 1.0f, 1.0f };
    //        if (entityMgr.HasComponent(entity, ScaleType)) {
    //            scale = entityMgr.GetComponent<Scale>(entity).axes;
    //        }

    //        // Calculate the EFFECTIVE size for the grid generation
    //        Vector2D finalHalfSize = { baseHalfSize.x * scale.x, baseHalfSize.y * scale.y };

    //        // 3. Get Rotation
    //        float rotation = 0.0f;
    //        if (entityMgr.HasComponent(entity, RotationFlagType)) {
    //            rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
    //        }
    //        else if (entityMgr.HasComponent(entity, RotationType)) {
    //            // Convert radians to degrees if necessary, or use angle
    //            rotation = entityMgr.GetComponent<Rotation>(entity).angle;
    //        }

    //        // 4. Calculate bounds for the loop using the FINAL Scaled Size
    //        // We manually calculate the AABB of the rotated object here to ensure 
    //        // we cover all necessary grid cells.

    //        float rotationRad = static_cast<float>(rotation * M_PI / 180.0f);
    //        float cos_r = std::cos(rotationRad);
    //        float sin_r = std::sin(rotationRad);

    //        // Calculate corners of the OBB
    //        float dx = finalHalfSize.x;
    //        float dy = finalHalfSize.y;

    //        // Project corners to find AABB min/max
    //        // Corner 1: ( dx,  dy)
    //        // Corner 2: (-dx,  dy)
    //        // We only need to check these two combinations for AABB extents due to symmetry
    //        float x1 = dx * cos_r - dy * sin_r;
    //        float y1 = dx * sin_r + dy * cos_r;
    //        float x2 = -dx * cos_r - dy * sin_r;
    //        float y2 = -dx * sin_r + dy * cos_r;

    //        float maxX = std::max({ std::abs(x1), std::abs(x2) });
    //        float maxY = std::max({ std::abs(y1), std::abs(y2) });

    //        Vector2D minAABB = pos - Vector2D(maxX, maxY);
    //        Vector2D maxAABB = pos + Vector2D(maxX, maxY);

    //        // Add padding
    //        float padding = cellSize * 0.5f;
    //        Vector2D gridMin = WorldToGrid(minAABB - Vector2D(padding, padding));
    //        Vector2D gridMax = WorldToGrid(maxAABB + Vector2D(padding, padding));

    //        // 5. Loop through cells
    //        for (int x = static_cast<int>(gridMin.x); x <= static_cast<int>(gridMax.x); ++x) {
    //            for (int y = static_cast<int>(gridMin.y); y <= static_cast<int>(gridMax.y); ++y) {

    //                Vector2D cellCenterWorld = GridToWorld(x, y);
    //                Vector2D p_local = cellCenterWorld - pos;

    //                // Rotate cell center into box's local space (Inverse rotation)
    //                // Inverse of rotation matrix is just transposing the sin term
    //                Vector2D p_rot = {
    //                    p_local.x * cos_r + p_local.y * sin_r,
    //                   -p_local.x * sin_r + p_local.y * cos_r
    //                };

    //                // Check overlap using the CALCULATED FinalHalfSize
    //                // We add a small margin to make walls slightly "thicker" to the AI
    //                // so they don't clip corners.
    //                float checkMargin = cellSize * 0.25f;

    //                if (p_rot.x >= -(finalHalfSize.x + checkMargin) &&
    //                    p_rot.x <= (finalHalfSize.x + checkMargin) &&
    //                    p_rot.y >= -(finalHalfSize.y + checkMargin) &&
    //                    p_rot.y <= (finalHalfSize.y + checkMargin))
    //                {
    //                    wallNodes_.insert({ x, y });
    //                }
    //            }
    //        }
    //    }
    //}

// In Pathfinding.cpp

// Scans all platform entities and marks grid cells they occupy as wall nodes.
// Accounts for rotation and scale using an oriented bounding-box projection.
void NavigationGrid::BuildGrid() {
    wallNodes_.clear();
    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // Find all entities that are platforms
    for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlatformTagType)) {

        if (!entityMgr.HasComponent(entity, TransformType)) continue;
        const Transform& transform = entityMgr.GetComponent<Transform>(entity);
        Vector2D pos = transform.position;

        // use aabbcollider as source of size
        Vector2D baseHalfSize = { 50.0f, 50.0f };

        if (entityMgr.HasComponent(entity, AABBColliderType)) {
            baseHalfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;
        }
        else if (entityMgr.HasComponent(entity, TextureComponentType)) {
            baseHalfSize = entityMgr.GetComponent<TextureComponent>(entity).HalfSize;
        }

        // Apply Scale for updating the aabbcollider when changing
        Vector2D scale = transform.scale;
        
        Vector2D finalHalfSize = { baseHalfSize.x * scale.x, baseHalfSize.y * scale.y };

        // Get Rotation
        float rotation = 0.0f;
        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
        }
        else {
            rotation = transform.rotation.angle;
        }

        //Calculate AABB of the Rotated Object 
        float rotationRad = static_cast<float>(rotation * M_PI / 180.0f);
        float cos_r = std::cos(rotationRad);
        float sin_r = std::sin(rotationRad);

        float dx = finalHalfSize.x;
        float dy = finalHalfSize.y;

        
        float x1 = dx * cos_r - dy * sin_r;
        float y1 = dx * sin_r + dy * cos_r;
        float x2 = -dx * cos_r - dy * sin_r;
        float y2 = -dx * sin_r + dy * cos_r;

        float maxX = std::max({ std::abs(x1), std::abs(x2) });
        float maxY = std::max({ std::abs(y1), std::abs(y2) });

        // padding for error margin
        float loopPadding = cellSize * 0.1f;
        Vector2D minAABB = pos - Vector2D(maxX, maxY);
        Vector2D maxAABB = pos + Vector2D(maxX, maxY);

        Vector2D gridMin = WorldToGrid(minAABB - Vector2D(loopPadding, loopPadding));
        Vector2D gridMax = WorldToGrid(maxAABB + Vector2D(loopPadding, loopPadding));

       
        for (int x = static_cast<int>(gridMin.x); x <= static_cast<int>(gridMax.x); ++x) {
            for (int y = static_cast<int>(gridMin.y); y <= static_cast<int>(gridMax.y); ++y) {

                Vector2D cellCenterWorld = GridToWorld(x, y);
                Vector2D p_local = cellCenterWorld - pos;

                // Rotate cell center into box's local space 
                Vector2D p_rot = {
                    p_local.x * cos_r + p_local.y * sin_r,
                   -p_local.x * sin_r + p_local.y * cos_r
                };

                // EXACT BOX CHECK
                if (p_rot.x >= -finalHalfSize.x && p_rot.x <= finalHalfSize.x &&
                    p_rot.y >= -finalHalfSize.y && p_rot.y <= finalHalfSize.y)
                {
                    wallNodes_.insert({ x, y });
                }
            }
        }
    }
}

    // Returns true if the cell at (x, y) is not occupied by a platform wall.
    bool NavigationGrid::IsWalkable(int x, int y) const {
        return wallNodes_.find({ x, y }) == wallNodes_.end(); 
    }

    // Converts a world-space position to its corresponding grid cell (column, row).
    Vector2D NavigationGrid::WorldToGrid(const Vector2D& worldPos) const {
        return {
            static_cast<float>(std::floor(worldPos.x / cellSize)),
            static_cast<float>(std::floor(worldPos.y / cellSize))
        };
    }

    // Returns the world-space center position for a given grid cell.
    Vector2D NavigationGrid::GridToWorld(int x, int y) const {
        return {
            (static_cast<float>(x) + 0.5f) * cellSize,
            (static_cast<float>(y) + 0.5f) * cellSize
        };
    }

    // Returns all walkable orthogonal neighbors (up, down, left, right) of the given node.
    // Diagonal movement is not allowed.
    std::vector<std::unique_ptr<PathNode>> NavigationGrid::GetNeighbors(PathNode* node)  {
        std::vector<std::unique_ptr<PathNode>> neighbors;
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                if (x == 0 && y == 0) continue; // Skip self

                // Disable diagonal movement
                if (x != 0 && y != 0) continue;

                int checkX = node->x + x;
                int checkY = node->y + y;

                if (IsWalkable(checkX, checkY)) {
                    neighbors.push_back(std::make_unique<PathNode>(checkX, checkY));
                }
            }
        }
        return neighbors;
    }

    // --- AStarPather Implementation ---

    // Manhattan distance heuristic — fast approximation of the remaining path cost.
    int AStarPather::Heuristic(const Vector2D& current, const Vector2D& goal) {
        // Manhattan distance 
        return static_cast<int>(std::abs(current.x - goal.x) + std::abs(current.y - goal.y));
    }

    // Retraces the parent chain from the goal node back to start, producing an ordered path.
    std::vector<Vector2D> AStarPather::RetracePath(NavigationGrid& navGrid, PathNode* endNode) {
        std::vector<Vector2D> path;
        PathNode* currentNode = endNode;
        while (currentNode != nullptr) {
            path.push_back(navGrid.GridToWorld(currentNode->x, currentNode->y));
            currentNode = currentNode->parent;
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    // Runs A* pathfinding between two world-space positions.
    // Returns an ordered list of world waypoints, or empty if no path exists.
    std::vector<Vector2D> AStarPather::FindPath(NavigationGrid& navGrid, Vector2D startWorld, Vector2D endWorld) {
        Vector2D startGrid = navGrid.WorldToGrid(startWorld);
        Vector2D endGrid = navGrid.WorldToGrid(endWorld);

        std::unique_ptr<Framework::PathNode> startNode = std::make_unique<PathNode>(static_cast<int>(startGrid.x), static_cast<int>(startGrid.y));
        std::unique_ptr<Framework::PathNode> endNode = std::make_unique<PathNode>(static_cast<int>(endGrid.x), static_cast<int>(endGrid.y));

        if (!navGrid.IsWalkable(endNode->x, endNode->y)) {
            return {}; // Can't path to a wall
        }

        std::map<std::pair<int, int>, std::unique_ptr<PathNode>> allNodes;
        std::priority_queue<PathNode*, std::vector<PathNode*>, ComparePathNode> openSet;
        std::set<std::pair<int, int>> closedSet;

        startNode->gCost = 0;
        startNode->hCost = Heuristic(startGrid, endGrid);

        openSet.push(startNode.get());
        allNodes[{startNode->x, startNode->y}] = std::move(startNode);

        while (!openSet.empty()) {
            PathNode* currentNode = openSet.top();
            openSet.pop();

            if (currentNode->x == endNode->x && currentNode->y == endNode->y) {
                return RetracePath(navGrid, currentNode); // Found path
            }

            closedSet.insert({ currentNode->x, currentNode->y });

            for ( std::vector<std::unique_ptr<Framework::PathNode>>::iterator::value_type& neighborNodePtr : navGrid.GetNeighbors(currentNode)) {
                if (closedSet.count({ neighborNodePtr->x, neighborNodePtr->y })) {
                    continue;
                }

                int newGCost = currentNode->gCost + Heuristic({ static_cast<float>(currentNode->x), static_cast<float>(currentNode->y) }, { static_cast<float>(neighborNodePtr->x), static_cast<float>(neighborNodePtr->y) });

                auto it = allNodes.find({ neighborNodePtr->x, neighborNodePtr->y });
                PathNode* neighbor = nullptr;

                if (it == allNodes.end() || newGCost < it->second->gCost) {
                    if (it == allNodes.end()) {
                        neighborNodePtr->gCost = newGCost;
                        neighborNodePtr->hCost = Heuristic({ static_cast<float>(neighborNodePtr->x), static_cast<float>(neighborNodePtr->y) }, endGrid);
                        neighborNodePtr->parent = currentNode;

                        neighbor = neighborNodePtr.get();
                        allNodes[{neighbor->x, neighbor->y}] = std::move(neighborNodePtr);
                        openSet.push(neighbor);
                    }
                    else {
                        neighbor = it->second.get();
                        neighbor->gCost = newGCost;
                        neighbor->hCost = Heuristic({ static_cast<float>(neighbor->x), static_cast<float>(neighbor->y) }, endGrid);
                        neighbor->parent = currentNode;

                        // Re-push to update priority
                        std::vector<PathNode*> temp;
                        while (!openSet.empty()) {
                            temp.push_back(openSet.top());
                            openSet.pop();
                        }
                        for (Framework::PathNode* node : temp) {
                            if (node != neighbor) openSet.push(node);
                        }
                        openSet.push(neighbor);
                    }
                }
            }
        }
        return {}; // No path found
    }
} // namespace Framework

