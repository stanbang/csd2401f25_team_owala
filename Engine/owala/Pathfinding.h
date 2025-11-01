#pragma once
#include "Vector2D.h"
#include <vector>
#include <set>
#include <map>
#include <memory>
#include "EntityStorage.h" // Need this to build the grid

namespace Framework {

    // Internal node representation for A*
    struct PathNode {
        int x, y; // Grid coordinates
        int gCost = 0; // Cost from start
        int hCost = 0; // Heuristic cost to end
        PathNode* parent = nullptr;

        PathNode(int x, int y) : x(x), y(y) {}
        int fCost() const { return gCost + hCost; }

        bool operator==(const PathNode& other) const {
            return x == other.x && y == other.y;
        }
    };

    /**
     * @class NavigationGrid
     * @brief Builds and stores a grid representation of the level for pathfinding.
     */
    class NavigationGrid {
    public:
        // Size of one grid cell in world coordinates
        // (editor already has a grid built in, but we want to make sure editor and game logic is seperate)
        const float cellSize = 0.1f;

        /**
         * @brief Scans all static colliders (PlatformTag) in the ECS to build a grid of "walls".
         * @param ecs The EntityStorage to scan.
         */
        void BuildGrid(EntityStorage& ecs);

        /**
         * @brief Checks if a grid cell is walkable (i.e., not a wall).
         */
        bool IsWalkable(int x, int y) const;

        /**
         * @brief Gets all valid, walkable neighbors of a given node.
         */
        std::vector<std::unique_ptr<PathNode>> GetNeighbors(PathNode* node);



        Vector2D WorldToGrid(const Vector2D& worldPos) const;
        Vector2D GridToWorld(int x, int y) const;

    private:
        // Set of grid coordinates that are blocked by walls
        std::set<std::pair<int, int>> wallNodes_;
    };


    /**
     * @class AStarPather
     * @brief Finds a path using the A* algorithm on a NavigationGrid.
     */
    class AStarPather {
    public:
        /**
         * @brief Finds a path between two world-space positions.
         * @return A vector of world-space waypoints. Empty if no path is found.
         */
        std::vector<Vector2D> FindPath(NavigationGrid& navGrid, Vector2D startWorld, Vector2D endWorld);

    private:
        int Heuristic(const Vector2D& a, const Vector2D& b);
        std::vector<Vector2D> RetracePath(NavigationGrid& navGrid, PathNode* endNode);
    };

} // namespace Framework

