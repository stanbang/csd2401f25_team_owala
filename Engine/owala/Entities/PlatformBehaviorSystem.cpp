/*******************************************************************
* \file   PlatformBehaviorSystem.cpp
*
* \primary_author
*      Sitara Anil
* \secondary_author
*      
* \brief
*      Implementation of the PlatformBehaviorSystem. Handles trap platform
*      trigger delays, falling mechanics, and state resets. 
* 
* 
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "PlatformBehaviorSystem.h"
#include "TransformationSystem.h"
#include "EntityManager.h"
#include "Components.h"
#include "Transformation.h"
#include "OwalaTime.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include <AABB_Collision.h>

namespace Framework {
    /*!
    @brief      Main update function that processes all special platform types
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::Update() {
        UpdateTrapPlatforms();
    }

    // ========== TRAP PLATFORM LOGIC ==========

    /*!
    @brief      Checks if a trap platform should fall based on its fall direction and world rotation
    @param      fallDirection The trap's fall direction vector
    @param      worldRotation Current world rotation in degrees
    @return     true if the trap should fall
*//*______________________________________________________________*/
    bool PlatformBehaviorSystem::ShouldTrapFall(EntityManager& entityMgr, Entity entity, float worldRotation) const
    {
        std::string entityName = entityMgr.GetProperties(entity).name;
        float normalizedRotation = std::fmod(worldRotation + 360.0f, 360.0f);

        // MultiAxis can always fall at 0 to 360 degrees
        if (entityName.find("MultiAxis") != std::string::npos)
        {
            if (std::abs(normalizedRotation) < 0.1f || std::abs(normalizedRotation - 90.0f) < 0.1f ||
                std::abs(normalizedRotation - 180.0f) < 0.1f || std::abs(normalizedRotation - 270.0f) < 0.1f ||
                std::abs(normalizedRotation - 360.0f) < 0.1f)
            {
                return true;
            }
            return false;
        }
        // Vertical traps fall at 0, 180, 360 degrees
        if (entityName.find("Vertical") != std::string::npos)
        {
            if (std::abs(normalizedRotation) < 0.1f || std::abs(normalizedRotation - 180.0f) < 0.1f ||
                std::abs(normalizedRotation - 360.0f) < 0.1f)
            {
                return true;
            }
            return false;
        }
        // Horizontal traps fall at 90, 270 degrees
        if (entityName.find("Horizontal") != std::string::npos)
        {
            if (std::abs(normalizedRotation - 90.0f) < 0.1f || std::abs(normalizedRotation - 270.0f) < 0.1f)
            {
                return true;
            }
            return false;
        }
        // Default: no fall
        return false;
    }

    Vector2D GetFallDirection(const std::string& entityName, float worldRotation)
    {
        // All traps fall downward in world space (gravity direction)
        return Vector2D(0.0f, -1.0f);
    }

    /*!
    @brief      Updates all trap platforms, handling trigger delays and falling
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void PlatformBehaviorSystem::UpdateTrapPlatforms() {
        EntityManager entityMgr(*ecs_);

        // Get current world rotation
        float worldRotation = transformSystem_->GetGlobalRotation();
        worldRotation = std::fmod(worldRotation + 360.0f, 360.0f);

   
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(entity);
            std::string entityName = entityMgr.GetProperties(entity).name;

            // Determine if trap should fall at current rotation
            bool shouldFall = ShouldTrapFall(entityMgr, entity, worldRotation);

            // If rotation changed and this trap should no longer fall, stop it
            if (trapComp.hasFallen && !shouldFall) {
                // Stop the trap from falling
                if (entityMgr.HasComponent(entity, VelocityType)) {
                    entityMgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
                }
                // Make it static again
                if (entityMgr.HasComponent(entity, RigidBodyType)) {
                    entityMgr.GetComponent<RigidBody>(entity).SetMass(0.0f);
                }
                // Reset fallen state but keep it triggered
                trapComp.hasFallen = false;
                continue;
            }

            // Skip if already fallen and should still be falling
            if (trapComp.hasFallen && shouldFall) {
                // Continue applying falling velocity
                if (entityMgr.HasComponent(entity, VelocityType)) {
                    Vector2D fallDir = GetFallDirection(entityName, worldRotation);
                    entityMgr.GetComponent<Velocity>(entity).Vel = fallDir * trapComp.fallSpeed;
                }
                continue;
            }

            // Trigger the trap if conditions are met and not already triggered
            if (!trapComp.isTriggered && shouldFall) {
                trapComp.isTriggered = true;
                trapComp.currentDelay = 0.0f;
                //std::cout << "TRAP TRIGGERED: " << entityName << " at rotation " << worldRotation << "\n";
            }

            // Reset trigger if rotation changed before falling
            if (trapComp.isTriggered && !trapComp.hasFallen && !shouldFall) {
                trapComp.isTriggered = false;
                trapComp.currentDelay = 0.0f;
                //std::cout << "TRAP RESET: " << entityName << " - rotation changed before falling\n";
                continue;
            }

            // Handle trigger delay countdown
            if (trapComp.isTriggered && !trapComp.hasFallen && shouldFall) {
                trapComp.currentDelay += Framework::Time::DeltaTime();

                if (trapComp.currentDelay >= trapComp.triggerDelay) {
                    trapComp.hasFallen = true;
                    if (entityMgr.HasComponent(entity, RigidBodyType)) {
                        entityMgr.GetComponent<RigidBody>(entity).SetMass(1.0f);
                    }
                }
            }

            // Apply falling velocity only if should be falling
            if (trapComp.hasFallen && shouldFall && entityMgr.HasComponent(entity, VelocityType)) {
                Vector2D fallDir = GetFallDirection(entityName, worldRotation);
                entityMgr.GetComponent<Velocity>(entity).Vel = fallDir * trapComp.fallSpeed;
            }
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

}