#include "ScalingPlatformSystem.h"
#include "ScalingPlatform.h"

namespace Framework {
    void ScalingPlatformSystem::Update(float dt) {
        updateScalingPlatforms(*ecs_, dt);
    }
}