/*****************************************************************//**
* \file   PhysicsSystem.h
*
* \primary_author
*      Lur Pei Hao Stanley
*
*
* \brief
* Declares the PhysicsSystem class, managing forces, integration, 
* and collisions. Defines the broad-phase grid constants and public 
* API for debug toggles and layer management.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Components.h"
#include "Collision.h"
#include <vector>
#include <bitset> // bitset is for storing binary data IE. layercollider data
#include <set> //  For duplicate pair checking
#include <unordered_map> // For Entity -> bitset index mapping
#include <algorithm>

namespace Framework {


    /**
     * @struct PhysicsDebugCell
     * @brief Holds data for rendering one cell of the physics grid.
     */
    struct PhysicsDebugCell {
        Vector2D worldMin;
        Vector2D worldMax;
        bool isActive = false;
    };

    /**
     * @class PhysicsSystem
     * @brief Manages all physics-related simulation for the engine.
     * @details This system is responsible for applying forces (like gravity),
     * updating positions and velocities (integration), and detecting
     * and resolving collisions between entities with physics components.
     */
    class PhysicsSystem : public ISystem {
    public:
        /**
         * @brief Constructs the PhysicsSystem.
         * @param storage A reference to the central EntityStorage.
         */
        PhysicsSystem() = default;


        /**
         * @brief Initializes the physics system .
         */
        void Initialize() override; 

        /**
         * @brief Runs the physics simulation for a given time delta.
         * @details Uses a fixed timestep internally for stability.
         * @param dt The time elapsed since the last frame.
         */
        void Update() override;

        /**
         * @brief Shuts down the physics system (currently not in use).
         */
        void Shutdown() override {}

        // --- Public Properties ---
        Vector2D gravity_ = { 0.0f, -981.0f }; // Public for easy access/modification (e.g., world rotation)
        static AABB_Bounds GetRotatedAABB(const Vector2D& center, const Vector2D& halfSize, float rotationDegrees);


        // Checks for overlapping collision triggers (wincons)
        static bool CheckTriggerOverlap(Entity e1, Entity e2);

        // --- Layer-Based Collision API ---

       /**
        * @brief Gets layer collision for the Editor
        * @param layer1 The first collision layer.
        * @param layer2 The second collision layer.
        * 
        */
        bool GetLayerCollision(CollisionLayer layer1, CollisionLayer layer2) const;


        // Returns layer count of collision layers
        static size_t GetLayerCount() { return static_cast<size_t>(CollisionLayer::Count); }

        /**
        * @brief Configures the collision interaction between two layers.
        * @param layer1 The first collision layer.
        * @param layer2 The second collision layer.
        * @param canCollide True if objects on these layers should collide, false otherwise.
        */
        void SetLayerCollision(CollisionLayer layer1, CollisionLayer layer2, bool canCollide);

        // --- Public Methods ---

         /**
         * @brief Pauses or unpauses the entire physics simulation.
         * @param paused True to pause, false to resume.
         */
        void SetPaused(bool paused) { isPaused_ = paused; }

        /**
         * @brief Pauses or unpauses both physics and animation updates.
         * @param paused True to pause animations, false to resume.
		 */
        void SetFullyPaused(bool paused) { isPaused_ = paused; isAnimationPaused_ = paused; }

        // Debugging controls
         /**
         * @brief Toggles the step-by-step debug mode.
         */
        void ToggleStepMode() { stepModeActive_ = !stepModeActive_; isPaused_ = false; }

        /**
         * @brief If in step mode, executes a single frame of the physics simulation.
         */
        void ExecuteNextStep() { if (stepModeActive_) { Step(1.0f / 60.0f); } }

        /**
         * @brief Checks if the simulation is currently paused.
         * @return True if paused, false otherwise.
         */
        bool IsPaused() const { return isPaused_; }

        /**
		 * @brief Pauses or unpauses animation updates (separate from physics pause).
		 * @return True if animation updates are paused, false otherwise.
         */
        bool IsAnimationPaused() const { return isAnimationPaused_; }

        /**
         * @brief Checks if the simulation is in step-by-step debug mode.
         * @return True if in step mode, false otherwise.
         */
        bool IsInStepMode() const { return stepModeActive_; }

        /**
         * @brief Forces the system to use the narrow-phase check.
         * @details For debugging and performance comparison.
         */
        void SetNarrowPhase(bool force) { forceNarrowPhase_ = force; }
        bool IsForcingNarrowPhase() const { return forceNarrowPhase_; }


        bool drawDebugGrid = false;

        /**
         * @brief Gathers all visible debug grid cells for the editor to draw.
         * @return A vector of cells with their world positions and active state.
         */
        std::vector<PhysicsDebugCell> GetDebugDrawData(Framework::CameraSystem* cameraSystem, Entity cameraID);


        /**
         * @brief Reload collision matrix from LayerManager after initialization
         * @details This should be called AFTER Initialize() to restore saved collision settings
         */
        void ReloadCollisionMatrixFromFile();


    private:
        /**
        * @brief Initializes the collision matrix with default values (all layers collide).
        */
        void InitializeCollisionMatrix();

        /**
         * @brief Executes a single, fixed-duration step of the physics simulation.
         * @param dt The fixed timestep (e.g., 1/60th of a second).
         */
        void Step(float dt);

        /**
         * @brief Updates the position and velocity of all dynamic objects based on forces.
         * @details Uses semi-implicit Euler integration:
         * velocity += (force / mass) * dt;
         * position += velocity * dt;
         * @param dt The fixed timestep.
         */
        void Integrate(float dt);

        /**
         * @brief Detects and resolves all collisions between objects for the current step.
         * @details Uses a stable, static-overlap resolution method based on minimum penetration.
         * @param dt The fixed timestep.
         */
        void ResolveCollisions(float dt);

        /**
         * @brief Performs the narrow-phase collision check and resolution
         * on a single pair of entities.
         * @param e1 The first entity.
         * @param e2 The second entity.
         * @param dt The fixed timestep.
         */
        void CheckCollisionPair(Entity e1, Entity e2, float dt);


        /**
         * @brief Detects and resolves all collisions between Player Entity and Viewport Bounds.
         * 
         */
        void EnforceViewportBounds();

        // --- Member Variables ---
        float timeAccumulator_ = 0.0f; // Used to implement a fixed timestep loop.
        bool isPaused_ = false;
		bool isAnimationPaused_ = false; // Separate pause for animations
        bool stepModeActive_ = false;
        std::vector<Contact> contacts_; // Stores info about collisions from the last frame.

        // The collision matrix. collisionMatrix_[layer1][layer2] is true if they can collide.
        std::vector<std::bitset<static_cast<size_t>(CollisionLayer::Count)>> collisionMatrix_;
        bool collisionMatrixInitialized_ = false; // collision matrix initialization boolean


        // ---  Broad-Phase Members using Fixed-Cell Grid ---

        // --- CONSTANTS FOR GRID ---
        static const int maxCollidableEntities = 1024; // Max entities for bitset (can add more)
        static constexpr float cellSize = 200.0f;     // World-space size of one cell
        static constexpr float invCellSize = 1.0f / cellSize;
        //  world bounds for the grid
        static constexpr float gridWorldMinX = -1920.0f;
        static constexpr float gridWorldMinY = -1920.0f;
        static constexpr float gridWorldWidth = 3840.0f;
        static constexpr float gridWorldHeight = 3840.0f;
        // grid dimensions (make sure grid isnt too big else no point doing broad phased implementation)
        static const int GridCellsX = static_cast<int>(gridWorldWidth * invCellSize) + 1;
        static const int GridCellsY = static_cast<int>(gridWorldHeight * invCellSize) + 1;

        // The grid using vector of bitsets. One bitset per cell
        std::vector<std::bitset<maxCollidableEntities>> grid_;

        // Mapping from Entity ID to bitset index (0 to MaxCollidableEntities-1)
        std::unordered_map<Entity, int> entityToColliderIndex_;
        // Mapping from bitset index -> Entity ID
        std::vector<Entity> colliderEntities_;

        // --- Grid Helper Functions ---

        /**
         * @brief Converts world X-coordinate to grid X-index, clamping to bounds.
         */
        int GetCellCoordX(float worldPos) const; 

        /**
         * @brief Converts world Y-coordinate to grid Y-index, clamping to bounds.
         */
        int GetCellCoordY(float worldPos) const ;

        /**
         * @brief Converts (x, y) grid index to a 1D vector index.
         */
        int GetCellIndex(int x, int y) const; 

        /**
         * @brief Inserts a collider's index into all overlapped grid cells.
         */
        void GridInsert(int colliderIndex, const AABB_Bounds& bounds); 

        /**
         * @brief Queries all overlapped cells and combines their bitsets.
         */
        void GridQuery(const AABB_Bounds& bounds, std::bitset<maxCollidableEntities>& potentialColliders) const;

        bool forceNarrowPhase_ = false; // Debug toggle for narrow or broad phased implementation

        
    
    };

} // namespace Framework