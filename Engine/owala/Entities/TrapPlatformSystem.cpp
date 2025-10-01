#include "TrapPlatformSystem.h"
#include "TrapPlatform.h"
#include "EntityManager.h"

namespace Framework {
    /*!
    @brief      Updates all trap platform entities, processing their state
                transitions and triggering logic
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void TrapPlatformSystem::Update(float dt) {
        EntityManager entityManager(*ecs_);
        updateTrapPlatforms(*ecs_, dt);
        for (const Entity& entity : entityManager.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            triggerTrapPlatform(*ecs_, entity);
        }
    }
}