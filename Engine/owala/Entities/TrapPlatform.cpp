#include "TrapPlatform.h"
#include "Platform.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {
    /*!
    @brief      Creates a trap platform entity at a specified position
    @param      entityStorage Reference to the entity storage system
    @param      pos Initial position of the trap platform
    @param      halfSize Half-dimensions of the platform's collision box
    @param      textureID String identifier for the platform's texture
    @return     Entity handle for the created trap platform
*//*______________________________________________________________*/
    Entity createTrapPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID)
    {
        EntityManager entityMgr(entityStorage);
        Entity trapPlatform = createPlatform(entityStorage, pos, halfSize, textureID);

        TrapPlatformComponent trapComponent;
        trapComponent.originalPosition = pos;
        trapComponent.isTriggered = false;
        trapComponent.hasFallen = false;
        trapComponent.currentDelay = 0.0f;
        trapComponent.currentFallDistance = 0.0f;
        trapComponent.fallSpeed = 2.0f;
        trapComponent.triggerDelay = 1.0f;

        entityMgr.AddComponent(trapPlatform, trapComponent);
        return trapPlatform;
    }
    /*!
    @brief      Updates all trap platforms in the scene, handling trigger delays
                and falling behavior
    @param      entityStorage Reference to the entity storage system
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void updateTrapPlatforms(EntityStorage& entityStorage, float dt)
    {
        EntityManager entityMgr(entityStorage);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(entity);
       
            if (trapComp.hasFallen) {
                continue;
            }

            // trigger delay
            if (trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.currentDelay += dt;
                if (trapComp.currentDelay >= trapComp.triggerDelay) {
                    trapComp.hasFallen = true;
                    if (entityMgr.HasComponent(entity, RigidBodyType)) {
                        entityMgr.GetComponent<RigidBody>(entity).SetMass(1.0f);
                    }
                }
            }

            if (trapComp.hasFallen && entityMgr.HasComponent(entity, PositionType) && entityMgr.HasComponent(entity, VelocityType)) {
                Vector2D currentVel = entityMgr.GetComponent<Velocity>(entity).Vel;

                if (currentVel.y == 0.0f) {                
                    continue;
                }

                // Fall down (negative Y direction)
                Vector2D fallVelocity = Vector2D(0.0f, -trapComp.fallSpeed);
                entityMgr.GetComponent<Velocity>(entity).Vel = fallVelocity;
            }
        }
    }
    /*!
    @brief      Triggers a trap platform to begin its falling sequence after
                the configured delay
    @param      entityStorage Reference to the entity storage system
    @param      trapPlatform Entity handle of the trap platform to trigger
    @return     void
*//*______________________________________________________________*/
    void triggerTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform)
    {
        EntityManager entityMgr(entityStorage);
        if (entityMgr.HasComponent(trapPlatform, TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapPlatform);
            if (!trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.isTriggered = true;
                trapComp.currentDelay = 0.0f;
            }
        }
    }
    /*!
    @brief      Resets a trap platform to its original state and position,
                ready to be triggered again
    @param      entityStorage Reference to the entity storage system
    @param      trapPlatform Entity handle of the trap platform to reset
    @return     void
*//*______________________________________________________________*/
    void resetTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform)
    {
        EntityManager entityMgr(entityStorage);
        if (entityMgr.HasComponent(trapPlatform, TrapPlatformComponentType) && entityMgr.HasComponent(trapPlatform, PositionType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapPlatform);
            trapComp.isTriggered = false;
            trapComp.hasFallen = false;
            trapComp.currentDelay = 0.0f;
            trapComp.currentFallDistance = 0.0f;

            entityMgr.GetComponent<Position>(trapPlatform).Pos = trapComp.originalPosition;

            if (entityMgr.HasComponent(trapPlatform, VelocityType)) {
                entityMgr.GetComponent<Velocity>(trapPlatform).Vel = Vector2D::Zero();
            }

            if (entityMgr.HasComponent(trapPlatform, RigidBodyType)) {
                entityMgr.GetComponent<RigidBody>(trapPlatform).SetMass(0.0f);
            }
        }
    }
}