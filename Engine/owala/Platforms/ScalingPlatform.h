#pragma once
#include "Entity.h"
#include "Vector2D.h"
#include "Matrix3x3.h"
#include <string>
#include <Components.h>

namespace Framework {
    struct ScalingPlatformComponent;
    class EntityStorage;
    Entity createScalingPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D baseHalfSize, std::string textureID, float scaleSpeed = 1.0f, float minScale = 0.5f, float maxScale = 2.0f);
    void updateScalingPlatforms(EntityStorage& entityStorage, float dt);
    Matrix3x3 getScalingPlatformTransform(EntityStorage& entityStorage, Entity entity);
}