/*****************************************************************//**
* \file   PhysicsSystem.h
*
* \primary_author
*      Lur Pei Hao Stanley
*
*
* \brief
* Declares the physics system, which manages forces, movement, and collisions.
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
#include <bitset>
// bitset is for storing binary data IE. layercollider data

namespace Framework {

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
        explicit PhysicsSystem(EntityStorage& storage);


        /**
         * @brief Initializes the physics system .
         */
        void Initialize() override; 

        /**
         * @brief Runs the physics simulation for a given time delta.
         * @details Uses a fixed timestep internally for stability.
         * @param dt The time elapsed since the last frame.
         */
        void Update(float dt) override;

        /**
         * @brief Shuts down the physics system (currently not in use).
         */
        void Shutdown() override {}

        // --- Public Properties ---
        Vector2D gravity_ = { 0.0f, -9.81f }; // Public for easy access/modification (e.g., world rotation)
        static AABB_Bounds GetRotatedAABB(const Vector2D& center, const Vector2D& halfSize, float rotationDegrees);

        // --- Layer-Based Collision API ---
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
         * @brief Checks if the simulation is in step-by-step debug mode.
         * @return True if in step mode, false otherwise.
         */
        bool IsInStepMode() const { return stepModeActive_; }

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
         * @brief Detects and resolves all collisions between Player Entity and Viewport Bounds.
         * 
         */
        void EnforceViewportBounds();

        // --- Member Variables ---
        EntityStorage* ecs_;
        float timeAccumulator_ = 0.0f; // Used to implement a fixed timestep loop.
        bool isPaused_ = false;
        bool stepModeActive_ = false;
        std::vector<Contact> contacts_; // Stores info about collisions from the last frame.

        // The collision matrix. collisionMatrix_[layer1][layer2] is true if they can collide.
        std::vector<std::bitset<static_cast<size_t>(CollisionLayer::Count)>> collisionMatrix_;
    };

} // namespace Framework