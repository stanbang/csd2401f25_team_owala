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

    void TrapPlatformSystem::Update(EntityStorage& ecs, float dt) {
        EntityManager entityManager(ecs);
        updateTrapPlatforms(ecs, dt);
    }
    }
