#include "PlatformBehaviorSystem.h"
#include "EntityManager.h"
#include "Components.h"
#include "Transformation.h"
#include "OwalaTime.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {
    /*!
    @brief      Main update function that processes all special platform types
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::Update() {
        TriggerAllTraps();
        UpdateTrapPlatforms();
        UpdateScalingPlatforms();
    }

    // ========== TRAP PLATFORM LOGIC ==========

    /*!
    @brief      Updates all trap platforms, handling trigger delays and falling
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::UpdateTrapPlatforms() {
        EntityManager entityMgr(*ecs_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(entity);

            // Skip platforms that have already fallen
            if (trapComp.hasFallen) {
                continue;
            }

            // Handle trigger delay countdown
            if (trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.currentDelay += Framework::Time::DeltaTime();

                // Check if delay period has elapsed
                if (trapComp.currentDelay >= trapComp.triggerDelay) {
                    trapComp.hasFallen = true;

                    // Give the platform mass so it can fall
                    if (entityMgr.HasComponent(entity, RigidBodyType)) {
                        entityMgr.GetComponent<RigidBody>(entity).SetMass(1.0f);
                    }
                }
            }

            // Apply falling velocity to platforms that are falling
            if (trapComp.hasFallen && entityMgr.HasComponent(entity, VelocityType)) {
                Vector2D currentVel = entityMgr.GetComponent<Velocity>(entity).Vel;

                // Only update if platform is still moving (hasn't hit something)
                if (currentVel.y != 0.0f) {
                    Vector2D fallVelocity = Vector2D(0.0f, -trapComp.fallSpeed);
                    entityMgr.GetComponent<Velocity>(entity).Vel = fallVelocity;
                }
            }
        }
    }

    /*!
    @brief      Triggers a trap platform to begin its falling sequence
    @param      trapPlatform Entity handle of the trap platform
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::TriggerTrap(Entity trapPlatform) {
        EntityManager entityMgr(*ecs_);

        if (entityMgr.HasComponent(trapPlatform, TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapPlatform);

            // Only trigger if not already triggered or fallen
            if (!trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.isTriggered = true;
                trapComp.currentDelay = 0.0f;
            }
        }
    }
    // testing purposes to test the fall of TrapPlatforms
    void PlatformBehaviorSystem::TriggerAllTraps() {
        EntityManager entityMgr(*ecs_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            TriggerTrap(entity);
        }
    }


    /*!
    @brief      Resets a trap platform to its original state and position
    @param      trapPlatform Entity handle of the trap platform
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::ResetTrap(Entity trapPlatform) {
        EntityManager entityMgr(*ecs_);

        if (!entityMgr.HasComponent(trapPlatform, TrapPlatformComponentType) ||
            !entityMgr.HasComponent(trapPlatform, PositionType)) {
            return;
        }

        TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapPlatform);

        // Reset component state
        trapComp.isTriggered = false;
        trapComp.hasFallen = false;
        trapComp.currentDelay = 0.0f;
        trapComp.currentFallDistance = 0.0f;

        // Reset position to original
        entityMgr.GetComponent<Position>(trapPlatform).Pos = trapComp.originalPosition;

        // Stop movement
        if (entityMgr.HasComponent(trapPlatform, VelocityType)) {
            entityMgr.GetComponent<Velocity>(trapPlatform).Vel = Vector2D::Zero();
        }

        // Make it static again
        if (entityMgr.HasComponent(trapPlatform, RigidBodyType)) {
            entityMgr.GetComponent<RigidBody>(trapPlatform).SetMass(0.0f);
        }
    }

    // ========== SCALING PLATFORM LOGIC ==========

    /*!
    @brief      Updates all scaling platforms, animating their scale
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::UpdateScalingPlatforms() {
        EntityManager entityMgr(*ecs_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ScalingPlatformComponentType)) {
            ScalingPlatformComponent& scalingComp = entityMgr.GetComponent<ScalingPlatformComponent>(entity);

            // Advance animation time
            scalingComp.time += Framework::Time::DeltaTime();

            // Calculate scale using sine wave for smooth oscillation
            float sineValue = std::sin(scalingComp.time * scalingComp.scaleSpeed);
            float normalizedSine = (sineValue + 1.0f) * 0.5f; // Map from [-1,1] to [0,1]

            // Interpolate between min and max scale
            scalingComp.currentScaleX = scalingComp.minScale +
                (scalingComp.maxScale - scalingComp.minScale) * normalizedSine;
            scalingComp.currentScaleY = 1.0f; // Keep Y scale constant

            // Update collision box to match current scale
            if (entityMgr.HasComponent(entity, AABBColliderType)) {
                Vector2D newHalfSize = scalingComp.baseHalfSize;
                newHalfSize.x *= scalingComp.currentScaleX;
                entityMgr.GetComponent<AABBCollider>(entity).HalfSize = newHalfSize;
            }
            // Update texture size to match current scale
            if (entityMgr.HasComponent(entity, TextureComponentType)) {
                TextureComponent& texComp = entityMgr.GetComponent<TextureComponent>(entity);
                texComp.HalfSize.x = scalingComp.baseHalfSize.x * scalingComp.currentScaleX;
                texComp.HalfSize.y = scalingComp.baseHalfSize.y * scalingComp.currentScaleY;
            }

            // Update transformation matrix for rendering
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
    @brief      Retrieves the transformation matrix for a scaling platform
    @param      scalingPlatform Entity handle of the scaling platform
    @return     Matrix3x3 transformation matrix (identity if invalid)
*//*______________________________________________________________*/
    Matrix3x3 PlatformBehaviorSystem::GetScalingTransform(Entity scalingPlatform) const {
        EntityManager entityMgr(*ecs_);

        if (entityMgr.HasComponent(scalingPlatform, ScalingPlatformComponentType)) {
            return entityMgr.GetComponent<ScalingPlatformComponent>(scalingPlatform).transformMatrix;
        }

        return Matrix3x3(); // Return identity matrix
    }

}