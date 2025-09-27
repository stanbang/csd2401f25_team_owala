#include "TrapPlatformSystem.h"
#include "TrapPlatform.h"

namespace Framework {
    void TrapPlatformSystem::Update(float dt) {
        updateTrapPlatforms(*ecs_, dt);
        for (auto& [entity, trapComp] : ecs_->trapPlatform) {
            triggerTrapPlatform(*ecs_, entity);
        }
    }
}