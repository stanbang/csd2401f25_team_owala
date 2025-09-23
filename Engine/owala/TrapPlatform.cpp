#include "TrapPlatform.h"
#include "Platform.h"
#include "EntityStorage.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    Entity createTrapPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID)
    {
        Entity trapPlatform = createPlatform(entityStorage, pos, halfSize, textureID);

        TrapPlatformComponent trapComponent;
        trapComponent.originalPosition = pos;
        trapComponent.isTriggered = false;
        trapComponent.hasFallen = false;
        trapComponent.currentDelay = 0.0f;
        trapComponent.currentFallDistance = 0.0f;
        trapComponent.fallSpeed = 2.0f; 
        trapComponent.triggerDelay = 1.0f;

        entityStorage.trapPlatform[trapPlatform] = trapComponent;
        return trapPlatform;
    }

    void updateTrapPlatforms(EntityStorage& entityStorage, float dt)
    {
        const float maxFallDistance = 0.0f;

        for (auto& [entity, trapComp] : entityStorage.trapPlatform) {
            if (trapComp.hasFallen && trapComp.currentFallDistance >= maxFallDistance) {
                continue;
            }

            // trigger delay
            if (trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.currentDelay += dt;
                if (trapComp.currentDelay >= trapComp.triggerDelay) {
                    trapComp.hasFallen = true;
                    if (entityStorage.rigidBody.count(entity)) {
                        entityStorage.rigidBody[entity].SetMass(1.0f); 
                    }
                }
            }

            if (trapComp.hasFallen && entityStorage.position.count(entity) && entityStorage.velocity.count(entity)) {
                Vector2D currentVel = entityStorage.velocity[entity].Vel;

                if (currentVel.y == 0.0f) {
                    continue;
                }

                // Fall down (negative Y direction)
                Vector2D fallVelocity = Vector2D(0.0f, -trapComp.fallSpeed);
                entityStorage.velocity[entity].Vel = fallVelocity;
            }
        }
    }

    void triggerTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform)
    {
        if (entityStorage.trapPlatform.count(trapPlatform)) {
            auto& trapComp = entityStorage.trapPlatform[trapPlatform];
            if (!trapComp.isTriggered && !trapComp.hasFallen) {
                trapComp.isTriggered = true;
                trapComp.currentDelay = 0.0f;
            }
        }
    }

    void resetTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform)
    {
        if (entityStorage.trapPlatform.count(trapPlatform) && entityStorage.position.count(trapPlatform)) {
            auto& trapComp = entityStorage.trapPlatform[trapPlatform];
            trapComp.isTriggered = false;
            trapComp.hasFallen = false;
            trapComp.currentDelay = 0.0f;
            trapComp.currentFallDistance = 0.0f;

            entityStorage.position[trapPlatform].Pos = trapComp.originalPosition;

            if (entityStorage.velocity.count(trapPlatform)) {
                entityStorage.velocity[trapPlatform].Vel = Vector2D::Zero();
            }

            if (entityStorage.rigidBody.count(trapPlatform)) {
                entityStorage.rigidBody[trapPlatform].SetMass(0.0f);
            }
        }
    }
}