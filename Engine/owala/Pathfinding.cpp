#include "Pathfinding.h"
#include "EntityManager.h" // Assuming you have this from your other files
#include "EntityStorage.h" // For ComponentType enums
#include <algorithm>
#include <queue>
#include <map>

namespace Framework {

    // --- NavigationGrid Implementation ---

    void NavigationGrid::BuildGrid(EntityStorage& ecs) {
        wallNodes_.clear();
        EntityManager entityMgr(ecs);

        // Find all entities that are platforms
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlatformTagType)) {

            // Ensure it has collision and position data
            if (entityMgr.HasComponent(entity, AABBColliderType) &&
                entityMgr.HasComponent(entity, PositionType)) {

                const auto& pos = entityMgr.GetComponent<Position>(entity).Pos;
                const auto& halfSize = entityMgr.GetComponent<AABBCollider>(entity).HalfSize;

                // Get the world-space bounds of the collider
                Vector2D min = pos - halfSize;
                Vector2D max = pos + halfSize;

                // Convert world bounds to grid bounds
                Vector2D gridMin = WorldToGrid(min);
                Vector2D gridMax = WorldToGrid(max);

                // "Rasterize" the collider onto the grid
                for (int x = static_cast<int>(gridMin.x); x <= static_cast<int>(gridMax.x); ++x) {
                    for (int y = static_cast<int>(gridMin.y); y <= static_cast<int>(gridMax.y); ++y) {
                        wallNodes_.insert({ x, y });
                    }
                }
            }
        }
    }

    bool NavigationGrid::IsWalkable(int x, int y) const {
        return wallNodes_.find({ x, y }) == wallNodes_.end();
    }

    Vector2D NavigationGrid::WorldToGrid(const Vector2D& worldPos) const {
        return {
            static_cast<float>(std::floor(worldPos.x / cellSize)),
            static_cast<float>(std::floor(worldPos.y / cellSize))
        };
    }

    Vector2D NavigationGrid::GridToWorld(int x, int y) const {
        return {
            (static_cast<float>(x) + 0.5f) * cellSize,
            (static_cast<float>(y) + 0.5f) * cellSize
        };
    }

    std::vector<std::unique_ptr<PathNode>> NavigationGrid::GetNeighbors(PathNode* node) {
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

    int AStarPather::Heuristic(const Vector2D& current, const Vector2D& goal) {
        // Manhattan distance 
        return static_cast<int>(std::abs(current.x - goal.x) + std::abs(current.y - goal.y));
    }

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

    std::vector<Vector2D> AStarPather::FindPath(NavigationGrid& navGrid, Vector2D startWorld, Vector2D endWorld) {
        Vector2D startGrid = navGrid.WorldToGrid(startWorld);
        Vector2D endGrid = navGrid.WorldToGrid(endWorld);

        auto startNode = std::make_unique<PathNode>(static_cast<int>(startGrid.x), static_cast<int>(startGrid.y));
        auto endNode = std::make_unique<PathNode>(static_cast<int>(endGrid.x), static_cast<int>(endGrid.y));

        if (!navGrid.IsWalkable(endNode->x, endNode->y)) {
            return {}; // Can't path to a wall
        }

        std::map<std::pair<int, int>, std::unique_ptr<PathNode>> allNodes;
        auto compare = [](PathNode* a, PathNode* b) { return a->fCost() > b->fCost(); };
        std::priority_queue<PathNode*, std::vector<PathNode*>, decltype(compare)> openSet(compare);
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

            for (auto& neighborNodePtr : navGrid.GetNeighbors(currentNode)) {
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
                        for (auto* node : temp) {
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

