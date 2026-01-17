/*******************************************************************
* \file   TransformationSystem.h
*
* \primary_author
*       Sitara Anil
*     
* \secondary_author
*
*
* \brief    A system that manages real-time transformation of game
*           objects including translation, rotation, and scaling. 
*           Handles global world rotation mechanics using Q and E keys.
*           Computes transformation matrices
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"
#include "PhysicsSystem.h"
#include "Matrix3x3.h"

namespace Framework {

    class TransformationSystem : public ISystem {
    public:
        /*!
        * \brief Constructs the transformation system with required subsystems
        * \param ecs Reference to entity storage containing all game objects
        * \param input Reference to input system for key detection (Q/E for rotation)
        * \param physics Reference to physics system for pause control during rotation
        */
        TransformationSystem();

        /*!
        * \brief Initializes the transformation system
        */
        void Initialize() override;

        /*!
        * \brief Updates transformations and handles world rotation
        * \param dt Delta time in seconds since last frame
        */
        void Update() override;

        /*!
        * \brief Shuts down the transformation system
        */
        void Shutdown() override;

        /*!
        * \brief Gets the transformation matrix for a specific entity
        * \param entity Entity to get transformation for
        * \return Transformation matrix (identity if entity has no transform)
        */
        Matrix3x3 GetTransformMatrix(Entity entity) const;

        /*!
        * \brief Updates a single entity's transformation matrix
        * \param entity Entity to update
        */
        void UpdateEntityTransform(Entity entity);

        /*!
        * \brief Gets the current global world rotation angle
        * \return Current world rotation in degrees
        */
        float GetGlobalRotation() const { return globalRotation; }

        /*!
        * \brief Checks if world is currently rotating
        * \return True if world rotation animation is in progress
        */
        bool IsGlobalRotating() const { return isGlobalRotating; }

        /*!
        * \brief Reset global world rotation state
        */
        void ResetGlobalRotation();

        /*!
        * \brief Gets the original position of the entity
        * \return Original position of entity
        */
        static Vector2D GetOriginalPos(EntityManager& entityMgr, Entity entity);


    private:
        // Global rotation state
        float globalRotation;
        float targetGlobalRotation;
        bool isGlobalRotating;
        float rotationCooldown;
        float rotationCooldownDuration;

        /*!
        * \brief Handles input and triggers for world rotation
        * \param dt Delta time for input processing
        *
        * Detects Q/E key presses and initiates 90-degree world rotation.
        * Respects cooldown period to prevent input spam.
        */
        void ProcessRotationInput();

        /*!
        * \brief Updates global world rotation animation
        * \param dt Delta time for smooth interpolation
        *
        * Smoothly rotates the world and all entities toward target angle.
        * Pauses physics during rotation and resumes when complete.
        */
        void UpdateGlobalRotation();

        /*!
        * \brief Computes transformation matrix for an entity
        * \param entity Entity to compute transform for
        * \return Computed transformation matrix (T * R * S)
        *
        * Internal helper that reads Position, RotationFlag, and Scale
        * components and generates the combined transformation matrix.
        */
        Matrix3x3 ComputeTransformMatrix(Entity entity) const;

        /*!
        * \brief Updates all entity transformation matrices
        * \param dt Delta time
        *
        * Recalculates transformation matrices for all active entities
        * with position and rotation components.
        */
        void UpdateAllTransforms();

        std::unordered_map<Entity, Vector2D> originalColliderSizes_;
    };
} // namespace Framework