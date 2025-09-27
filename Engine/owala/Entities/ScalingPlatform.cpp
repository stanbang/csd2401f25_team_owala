#include "ScalingPlatform.h"
#include "Platform.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include "Transformation.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    Entity createScalingPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D baseHalfSize, std::string textureID, float scaleSpeed, float minScale, float maxScale)
    {
		EntityManager entityMgr(entityStorage);
        Entity scalingPlatform = createPlatform(entityStorage, pos, baseHalfSize, textureID);
        ScalingPlatformComponent scalingComp;
        scalingComp.baseHalfSize = baseHalfSize;
        scalingComp.currentScaleX = 1.0f;
        scalingComp.currentScaleY = 1.0f;
        scalingComp.scaleSpeed = scaleSpeed;
        scalingComp.minScale = minScale;
        scalingComp.maxScale = maxScale;
        scalingComp.time = 0.0f;

        entityMgr.AddComponent(scalingPlatform, scalingComp);
        return scalingPlatform;
    }

    void updateScalingPlatforms(EntityStorage& entityStorage, float dt)
    {
		EntityManager entityMgr(entityStorage);
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ScalingPlatformComponentType)) {
			ScalingPlatformComponent& scalingComp = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
            scalingComp.time += dt;


            float sineValue = std::sin(scalingComp.time * scalingComp.scaleSpeed);
            float normalizedSine = (sineValue + 1.0f) * 0.5f;

            scalingComp.currentScaleX = scalingComp.minScale + (scalingComp.maxScale - scalingComp.minScale) * normalizedSine;
            scalingComp.currentScaleY = 1.0f;

            if (entityMgr.HasComponent(entity, AABBColliderType)) {
                Vector2D newHalfSize = scalingComp.baseHalfSize;
                newHalfSize.x *= scalingComp.currentScaleX;
                entityMgr.GetComponent<AABBCollider>(entity).HalfSize = newHalfSize;
            }

            if (entityMgr.HasComponent(entity, PositionType)) {
                Vector2D position = entityMgr.GetComponent<Position>(entity).Pos;
                float rotation = 0.0f;
                if (entityMgr.HasComponent(entity, RotationFlagType)) {
                    rotation = entityMgr.GetComponent<RotationFlag>(entity).currentRotation;
                }

                Vector2D renderScale = Vector2D(scalingComp.currentScaleX, scalingComp.currentScaleY);
                scalingComp.transformMatrix = Transformation::CreateTransformationMatrix(position, rotation, renderScale);
            }
        }
    }

    Matrix3x3 getScalingPlatformTransform(EntityStorage& entityStorage, Entity entity) {
		EntityManager entityMgr(entityStorage);
        if (entityMgr.HasComponent(entity, ScalingPlatformComponentType)) {
            return entityMgr.GetComponent<ScalingPlatformComponent>(entity).transformMatrix;
        }
        return Matrix3x3();
    }
}