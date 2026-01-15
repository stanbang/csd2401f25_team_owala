/*******************************************************************
* \file   TransformationSystem.cpp
*
* \primary_author
*       Sitara Anil
*
* \secondary_author
*
*
* \brief   
*       Implementation of the TransformationSystem. Handles global world
*       rotation triggered by Q/E keys with smooth interpolation and cooldown.
*       Computes and updates transformation matrices (T*R*S) for all entities.
*       Rotates entity positions around world origin and manages physics
*       pausing during rotation animations.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "TransformationSystem.h"
#include "GameManager.h"
#include "EntityManager.h"
#include "Components.h"
#include "Transformation.h"
#include <cmath>
#include <iostream>
#include "OwalaTime.h"
#include "MessageBus.h"

namespace Framework {

    TransformationSystem::TransformationSystem()
        : globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false), rotationCooldown(0.0f), rotationCooldownDuration(0.5f) {
    }

    /*!
    @brief      Initializes the transformation system and computes initial
                transformation matrices for all transformable entities
    @param      void
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::Initialize() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TransformType)) {
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                UpdateEntityTransform(entity);
            }
        }
    }

    /*!
    @brief      Main update function that handles both world rotation and
                entity transformation matrix updates
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::Update() {
        ProcessRotationInput();
        UpdateGlobalRotation();
        UpdateAllTransforms();
    }

    /*!
    @brief      Processes Q/E key input to trigger global world rotation
    @param      dt Delta time for cooldown updates
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::ProcessRotationInput() {
        GameManager& gm = GameManager::GetInstance();
        InputSystem& input_ = *gm.GetSystems().input_;
        PhysicsSystem& physics_ = *gm.GetSystems().physics_;
        EntityManager entityMgr(gm.GetECS());

        // Update cooldown timer
        if (rotationCooldown > 0.0f) {
            rotationCooldown -= Framework::Time::DeltaTime();
            if (rotationCooldown < 0.0f) {
                rotationCooldown = 0.0f;
            }
        }

        bool canRotate = false; // boolean for checking for rotation (for grounded player)

        // Find the player entity to check their state
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
            // Ensure we only check active players
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            // Get the player state updated by PhysicsSystem
            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);

            // Only allow rotation if the player is grounded
            if (state.isGrounded) {
                canRotate = true;
                break; // Found the player, we can proceed
            }
        }

        // If the player is in the air (not grounded), exit  function early
        if (!canRotate) {
            return;
        }

        if (!isGlobalRotating && rotationCooldown <= 0.0f) {
            if (input_.KeyPressed(InputSystem::Key::Q)) {
                // Q = anticlockwise = positive rotation
                targetGlobalRotation += 90.0f;
                physics_.SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "World rotating anticlockwise 90 degrees" << std::endl;
                MessageBus::Instance().Publish(WorldRotationEvent{}); // Publish world rotation event
            }

            if (input_.KeyPressed(InputSystem::Key::E)) {
                // E = clockwise = negative rotation
                targetGlobalRotation -= 90.0f;
                physics_.SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "World rotating clockwise 90 degrees" << std::endl;
				MessageBus::Instance().Publish(WorldRotationEvent{}); // Publish world rotation event
            }
        }
    }

/*!
    @brief      Updates the global world rotation, smoothly interpolating
                and rotating all entity positions around the world origin
    @param      dt Delta time for smooth rotation speed
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateGlobalRotation() {
        if (!isGlobalRotating) return;

        GameManager& gm = GameManager::GetInstance();
        InputSystem& input_ = *gm.GetSystems().input_;
        PhysicsSystem& physics_ = *gm.GetSystems().physics_;
        EntityManager entityMgr(gm.GetECS());

        float rotationSpeed = 270.0f; // Degrees per second
        float rotationThisFrame = rotationSpeed * Framework::Time::DeltaTime();

        float rotationIncrement = 0.0f;

        if (targetGlobalRotation > globalRotation) {
            // Rotating counterclockwise (Q key)
            rotationIncrement = std::min(rotationThisFrame, targetGlobalRotation - globalRotation);
            globalRotation += rotationIncrement;
        }
        else if (targetGlobalRotation < globalRotation) {
            // Rotating clockwise (E key)  
            rotationIncrement = -std::min(rotationThisFrame, globalRotation - targetGlobalRotation);
            globalRotation += rotationIncrement;
        }

        // Check if rotation is complete
        if (std::abs(targetGlobalRotation - globalRotation) < 0.1f) {
            globalRotation = targetGlobalRotation;
            isGlobalRotating = false;
            physics_.SetPaused(false);
            std::cout << "World rotation complete at " << globalRotation << " degrees" << std::endl;
        }

        if (std::abs(rotationIncrement) > 0.001f) {
            Matrix3x3 rotationMat = Transformation::CreateRotationMatrix(rotationIncrement);

            // Rotate all entities with RotationFlag around world origin (0,0)
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
                if (!entityMgr.HasComponent(entity, TransformType)) continue;
                if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

                Vector2D originalPos = entityMgr.GetComponent<Transform>(entity).position;
                entityMgr.GetComponent<Transform>(entity).position = rotationMat * originalPos;

                // Update entity's local rotation angle (except for players)
                if (!entityMgr.HasComponent(entity, PlayerTagType) && !entityMgr.HasComponent(entity, EnemyTagType)) {
                    RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    rotationFlag.currentRotation += rotationIncrement;
                    rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
                }
            }

            // Reset grounded state for player after rotation
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
                if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                state.isGrounded = true;
            }
        }
    }

    void TransformationSystem::ResetGlobalRotation() {
        globalRotation = 0.0f;
        targetGlobalRotation = 0.0f;
        isGlobalRotating = false;
        rotationCooldown = 0.0f;
        rotationCooldownDuration = 0.5f;
    }

    Vector2D TransformationSystem::GetOriginalPos(EntityManager& entityMgr, Entity entity) {

        // Check if the entity has the necessary components
        if (!entityMgr.HasComponent(entity, RotationFlagType) ||
            !entityMgr.HasComponent(entity, TransformType))
        {
            return Vector2D(0.0f, 0.0f);
        }

        Vector2D currentPos = entityMgr.GetComponent<Transform>(entity).position;
        
        const RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
        float totalRotationAngle = rotationFlag.currentRotation;

        float inverseAngle = -totalRotationAngle;
        Matrix3x3 inverseRotationMat = Transformation::CreateRotationMatrix(inverseAngle);

        Vector2D originalPos = inverseRotationMat * currentPos;

        return originalPos;
    }

    /*!
    @brief      Updates transformation matrices for all entities
    @param      dt Delta time
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateAllTransforms() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Update standard entities with position and rotation
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TransformType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;
            if (!entityMgr.HasComponent(entity, RotationFlagType)) continue;
            UpdateEntityTransform(entity);
        }

    }

/*!
    @brief      Updates the transformation matrix for a specific entity
                by computing T * R * S from its components
    @param      entity Entity to update transformation for
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateEntityTransform(Entity entity) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(entity, TransformType) ||
            !entityMgr.HasComponent(entity, RotationFlagType)) {
            return;
        }

        Transform& transform = entityMgr.GetComponent<Transform>(entity);
        Vector2D position = transform.position;

        if (entityMgr.HasComponent(entity, AABBColliderType)) {
            Vector2D& scale = entityMgr.GetComponent<Transform>(entity).scale;
            AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);

            if (originalColliderSizes_.find(entity) != originalColliderSizes_.end()) {
                Vector2D expectedSize = Vector2D(
                    originalColliderSizes_[entity].x * scale.x,
                    originalColliderSizes_[entity].y * scale.y
                );

                if (std::abs(collider.HalfSize.x - expectedSize.x) > 0.01f ||
                    std::abs(collider.HalfSize.y - expectedSize.y) > 0.01f) {
                    originalColliderSizes_.erase(entity);
                }
            }

            if (originalColliderSizes_.find(entity) == originalColliderSizes_.end()) {
                Vector2D baseSize = collider.HalfSize;
                if (scale.x != 0) baseSize.x /= scale.x;
                if (scale.y != 0) baseSize.y /= scale.y;
                originalColliderSizes_[entity] = baseSize;
            }

            collider.HalfSize.x = originalColliderSizes_[entity].x * scale.x;
            collider.HalfSize.y = originalColliderSizes_[entity].y * scale.y;
        }
    
        // Handle Texture Rotation
        Transform::Rotation& rotComp = transform.rotation;

        float textureRotationDegrees = rotComp.textureAngle * (180.0f / 3.14159265359f);
        float colliderRotationDegrees = rotComp.colliderAngle * (180.0f / 3.14159265359f);

        Vector2D textureScale = Vector2D(1.0f, 1.0f);
        if (entityMgr.HasComponent(entity, TextureComponentType)) {
            TextureComponent& tex = entityMgr.GetComponent<TextureComponent>(entity);
            textureScale = tex.HalfSize;
        }

        Vector2D& scale = entityMgr.GetComponent<Transform>(entity).scale;
        textureScale.x *= scale.x;
        textureScale.y *= scale.y;

        rotComp.textureTransformMatrix = Transformation::CreateTransformationMatrix(position, textureRotationDegrees, textureScale);

        if (entityMgr.HasComponent(entity, AABBColliderType))
        {
            //AABBCollider& collider = entityMgr.GetComponent<AABBCollider>(entity);
            Vector2D colliderScale = Vector2D(1.0f, 1.0f);
            colliderRotationDegrees = 0.0f;
            if (entityMgr.HasComponent(entity, RotationFlagType)) {
                colliderRotationDegrees = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
            }
            else {
                colliderRotationDegrees = rotComp.colliderAngle * (180.0f / 3.14159265359f);
            }

            // Apply scale to collider transform matrix
            Vector2D& newscale = entityMgr.GetComponent<Transform>(entity).scale;
            colliderScale.x = newscale.x;
            colliderScale.y = newscale.y;

            rotComp.colliderTransformMatrix = Transformation::CreateTransformationMatrix(position, colliderRotationDegrees, colliderScale);
        }

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
            float worldRotation = rotationFlag.currentRotation;

            Vector2D newscale = entityMgr.GetComponent<Transform>(entity).scale;

            rotationFlag.transformMatrix = Transformation::CreateTransformationMatrix(position, worldRotation, newscale);
        }
    }

    /*!
    @brief      Gets the transformation matrix for an entity
    @param      entity Entity to retrieve transformation matrix for
    @return     Matrix3x3 transformation matrix (identity if no transform)
*//*______________________________________________________________*/
    Matrix3x3 TransformationSystem::GetTransformMatrix(Entity entity) const {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Standard entity transformation
        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            return entityMgr.GetComponent<RotationFlag>(entity).transformMatrix;
        }

        return Matrix3x3();
    }

    /*!
    @brief      Computes transformation matrix from entity components
    @param      entity Entity to compute transformation for
    @return     Matrix3x3 computed transformation matrix (T * R * S)
*//*______________________________________________________________*/
    Matrix3x3 TransformationSystem::ComputeTransformMatrix(Entity entity) const {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(entity, TransformType)) {
            return Matrix3x3(); // Identity matrix
        }

        Vector2D position = entityMgr.GetComponent<Transform>(entity).position;
        float rotation = 0.0f;
        Vector2D scale = Vector2D(1.0f, 1.0f);

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
        }

        return Transformation::CreateTransformationMatrix(position, rotation, scale);
    }

    /*!
    @brief      Shuts down the transformation system
    @param      void
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::Shutdown() {
    }
}