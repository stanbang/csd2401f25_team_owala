#include "TransformationSystem.h"
#include "EntityManager.h"
#include "Components.h"
#include "Transformation.h"
#include <cmath>
#include <iostream>

namespace Framework {

    TransformationSystem::TransformationSystem(EntityStorage& ecs, InputSystem& input,PhysicsSystem& physics)
        : ecs_(&ecs), input_(&input), physics_(&physics),
        globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false), rotationCooldown(0.0f), rotationCooldownDuration(0.5f) {
    }

    /*!
    @brief      Initializes the transformation system and computes initial
                transformation matrices for all transformable entities
    @param      void
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::Initialize() {
        EntityManager entityMgr(*ecs_);
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PositionType)) {
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
    void TransformationSystem::Update(float dt) {
        ProcessRotationInput(dt);
        UpdateGlobalRotation(dt);
        UpdateAllTransforms(dt);
    }

    /*!
    @brief      Processes Q/E key input to trigger global world rotation
    @param      dt Delta time for cooldown updates
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::ProcessRotationInput(float dt) {
        // Update cooldown timer
        if (rotationCooldown > 0.0f) {
            rotationCooldown -= dt;
            if (rotationCooldown < 0.0f) {
                rotationCooldown = 0.0f;
            }
        }

        if (!isGlobalRotating && rotationCooldown <= 0.0f) {
            if (input_->KeyPressed(InputSystem::Key::Q)) {
                // Q = anticlockwise = positive rotation
                targetGlobalRotation += 90.0f;
                physics_->SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "World rotating anticlockwise 90 degrees" << std::endl;
            }

            if (input_->KeyPressed(InputSystem::Key::E)) {
                // E = clockwise = negative rotation
                targetGlobalRotation -= 90.0f;
                physics_->SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                std::cout << "World rotating clockwise 90 degrees" << std::endl;
            }
        }
    }

/*!
    @brief      Updates the global world rotation, smoothly interpolating
                and rotating all entity positions around the world origin
    @param      dt Delta time for smooth rotation speed
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateGlobalRotation(float dt) {
        if (!isGlobalRotating) return;

        EntityManager entityMgr(*ecs_);

        float rotationSpeed = 270.0f; // Degrees per second
        float rotationThisFrame = rotationSpeed * dt;

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
            physics_->SetPaused(false);
            std::cout << "World rotation complete at " << globalRotation << " degrees" << std::endl;
        }

        if (std::abs(rotationIncrement) > 0.001f) {
            Matrix3x3 rotationMat = Transformation::CreateRotationMatrix(rotationIncrement);

            // Rotate all entities with RotationFlag around world origin (0,0)
            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
                if (!entityMgr.HasComponent(entity, PositionType)) continue;
                if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

                Vector2D originalPos = entityMgr.GetComponent<Position>(entity).Pos;
                entityMgr.GetComponent<Position>(entity).Pos = rotationMat * originalPos;

                // Update entity's local rotation angle (except for players)
                if (!entityMgr.HasComponent(entity, PlayerTagType)) {
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

    /*!
    @brief      Updates transformation matrices for all entities
    @param      dt Delta time
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateAllTransforms(float dt) {
        EntityManager entityMgr(*ecs_);

        // Update standard entities with position and rotation
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PositionType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;
            if (!entityMgr.HasComponent(entity, RotationFlagType)) continue;
            UpdateEntityTransform(entity);
        }

        // Handle scaling platforms separately
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ScalingPlatformComponentType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            ScalingPlatformComponent& scalingComp =
                entityMgr.GetComponent<ScalingPlatformComponent>(entity);

            if (entityMgr.HasComponent(entity, PositionType)) {
                Vector2D position = entityMgr.GetComponent<Position>(entity).Pos;
                float rotation = 0.0f;

                if (entityMgr.HasComponent(entity, RotationFlagType)) {
                    rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
                }

                Vector2D renderScale = Vector2D(scalingComp.currentScaleX, scalingComp.currentScaleY);
                scalingComp.transformMatrix = Transformation::CreateTransformationMatrix(
                    position, rotation, renderScale
                );
            }
        }
    }

/*!
    @brief      Updates the transformation matrix for a specific entity
                by computing T * R * S from its components
    @param      entity Entity to update transformation for
    @return     void
*//*______________________________________________________________*/
    void TransformationSystem::UpdateEntityTransform(Entity entity) {
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(entity, PositionType) ||
            !entityMgr.HasComponent(entity, RotationFlagType)) {
            return;
        }

        // Get transform components
        Vector2D position = entityMgr.GetComponent<Position>(entity).Pos;
        float rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;

        Vector2D scale = Vector2D(1.0f, 1.0f);

        if (entityMgr.HasComponent(entity, ScalingPlatformComponentType)) {
            ScalingPlatformComponent& scalingComp =entityMgr.GetComponent<ScalingPlatformComponent>(entity);
            scale = Vector2D(scalingComp.currentScaleX, scalingComp.currentScaleY);
        }

        // Create transformation matrix: T * R * S
        Matrix3x3 transformMatrix = Transformation::CreateTransformationMatrix( position, rotation, scale );

        // Store the transformation matrix in the RotationFlag component
        entityMgr.GetComponent<RotationFlag>(entity).transformMatrix = transformMatrix;
    }

    /*!
    @brief      Gets the transformation matrix for an entity
    @param      entity Entity to retrieve transformation matrix for
    @return     Matrix3x3 transformation matrix (identity if no transform)
*//*______________________________________________________________*/
    Matrix3x3 TransformationSystem::GetTransformMatrix(Entity entity) const {
        EntityManager entityMgr(*ecs_);

        if (entityMgr.HasComponent(entity, ScalingPlatformComponentType)) {
            return entityMgr.GetComponent<ScalingPlatformComponent>(entity).transformMatrix;
        }

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
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(entity, PositionType)) {
            return Matrix3x3(); // Identity matrix
        }

        Vector2D position = entityMgr.GetComponent<Position>(entity).Pos;
        float rotation = 0.0f;
        Vector2D scale = Vector2D(1.0f, 1.0f);

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
        }

        if (entityMgr.HasComponent(entity, ScalingPlatformComponentType)) {
            ScalingPlatformComponent& scalingComp =entityMgr.GetComponent<ScalingPlatformComponent>(entity);
            scale = Vector2D(scalingComp.currentScaleX, scalingComp.currentScaleY);
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