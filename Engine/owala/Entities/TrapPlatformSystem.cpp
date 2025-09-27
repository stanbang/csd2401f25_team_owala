#include "TrapPlatformSystem.h"
#include "TrapPlatform.h"
#include "EntityManager.h"

namespace Framework {
    void TrapPlatformSystem::Update(float dt) {
        EntityManager entityManager(*ecs_);
        updateTrapPlatforms(*ecs_, dt);
        for (const Entity& entity : entityManager.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            triggerTrapPlatform(*ecs_, entity);
        }
    }
}