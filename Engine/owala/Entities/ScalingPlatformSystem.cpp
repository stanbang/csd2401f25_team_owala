#include "ScalingPlatformSystem.h"
#include "ScalingPlatform.h"

namespace Framework {
    /*!
    @brief      Updates all scaling platform entities, processing their
                animation and transformation calculations
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void ScalingPlatformSystem::Update(float dt) {
        updateScalingPlatforms(*ecs_, dt);
    }
}