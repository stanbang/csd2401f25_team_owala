#include "ScalingPlatform.h"
#include "Platform.h"
#include "EntityStorage.h"
#include "Transformation.h"
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Framework {

    Entity createScalingPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D baseHalfSize, std::string textureID, float scaleSpeed, float minScale, float maxScale)
    {
        Entity scalingPlatform = createPlatform(entityStorage, pos, baseHalfSize, textureID);
        ScalingPlatformComponent scalingComp;
        scalingComp.baseHalfSize = baseHalfSize;
        scalingComp.currentScaleX = 1.0f;
        scalingComp.currentScaleY = 1.0f;
        scalingComp.scaleSpeed = scaleSpeed;
        scalingComp.minScale = minScale;
        scalingComp.maxScale = maxScale;
        scalingComp.time = 0.0f;

        entityStorage.scalingPlatform[scalingPlatform] = scalingComp;
        return scalingPlatform;
    }

    void updateScalingPlatforms(EntityStorage& entityStorage, float dt)
    {
        for (auto& [entity, scalingComp] : entityStorage.scalingPlatform) {
            scalingComp.time += dt;


            float sineValue = std::sin(scalingComp.time * scalingComp.scaleSpeed);
            float normalizedSine = (sineValue + 1.0f) * 0.5f;

            scalingComp.currentScaleX = scalingComp.minScale + (scalingComp.maxScale - scalingComp.minScale) * normalizedSine;
            scalingComp.currentScaleY = 1.0f;

            if (entityStorage.colliders.count(entity)) {
                Vector2D newHalfSize = scalingComp.baseHalfSize;
                newHalfSize.x *= scalingComp.currentScaleX;
                entityStorage.colliders[entity].HalfSize = newHalfSize;
            }

            if (entityStorage.position.count(entity)) {
                Vector2D position = entityStorage.position[entity].Pos;
                float rotation = 0.0f;
                if (entityStorage.rotationFlag.count(entity)) {
                    rotation = entityStorage.rotationFlag[entity].currentRotation;
                }

                Vector2D renderScale = Vector2D(scalingComp.currentScaleX, scalingComp.currentScaleY);
                scalingComp.transformMatrix = Transformation::CreateTransformationMatrix(position, rotation, renderScale);
            }
        }
    }

    Matrix3x3 getScalingPlatformTransform(EntityStorage& entityStorage, Entity entity) {
        if (entityStorage.scalingPlatform.count(entity)) {
            return entityStorage.scalingPlatform[entity].transformMatrix;
        }
        return Matrix3x3();
    }
}