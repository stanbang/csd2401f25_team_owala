#pragma once
#include "Entity.h"
#include "Vector2D.h"
#include <string>
#include <Components.h>

namespace Framework {
    struct TrapPlatformComponent;
    class EntityStorage;
    Entity createTrapPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID);
    void updateTrapPlatforms(EntityStorage& entityStorage, float dt);
    void triggerTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform);
    void resetTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform);
}