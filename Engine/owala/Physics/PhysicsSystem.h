/*****************************************************************//**
* \file   PhysicsSystem.h
*
* \primary_author
*      Lur Pei Hao Stanley
* \secondary_author
*
*
* \brief
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Collision.h"
#include <vector>

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
        explicit PhysicsSystem(EntityStorage& storage);

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}

        // --- Public Properties ---
        Vector2D gravity_ = { 0.0f, -9.81f }; // Public for easy access/modification (e.g., world rotation)
        static AABB_Bounds GetRotatedAABB(const Vector2D& center, const Vector2D& halfSize, float rotationDegrees);

        // --- Public Methods ---
        void SetPaused(bool paused) { isPaused_ = paused; }

        // Debugging controls
        void ToggleStepMode() { stepModeActive_ = !stepModeActive_; isPaused_ = false; }
        void ExecuteNextStep() { if (stepModeActive_) { Step(1.0f / 60.0f); } }
        bool IsPaused() const { return isPaused_; }
        bool IsInStepMode() const { return stepModeActive_; }

    private:
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

        void EnforceViewportBounds();

        // --- Member Variables ---
        EntityStorage* ecs_;
        float timeAccumulator_ = 0.0f; // Used to implement a fixed timestep loop.
        bool isPaused_ = false;
        bool stepModeActive_ = false;
        std::vector<Contact> contacts_; // Stores info about collisions from the last frame.
    };

} // namespace Framework