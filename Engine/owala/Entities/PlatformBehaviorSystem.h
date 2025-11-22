/*******************************************************************
* \file   PlatformBehaviorSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*      System for managing special platform behaviors including trap
*      platforms that fall after a trigger delay and scaling platforms
*      that oscillate in size over time.
*
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Entity.h"
#include "Matrix3x3.h"

namespace Framework {
    class TransformationSystem;
}

namespace Framework {
    class PlatformBehaviorSystem : public ISystem {
    private:
        EntityStorage* ecs_;
        TransformationSystem* transformSystem_;
        bool ShouldTrapFall(EntityManager& entityMgr, Entity entity, float worldRotation) const;
        void UpdateTrapPlatforms();
        void UpdateScalingPlatforms();

    public:
        PlatformBehaviorSystem(EntityStorage& storage, TransformationSystem& transformSystem) : ecs_(&storage), transformSystem_(&transformSystem) {}

        void Initialize() override {}
        void Update() override;
        void Shutdown() override {}

        // Trap platform specific methods
        //void TriggerTrap(Entity trapPlatform);
        //void TriggerAllTraps();
        void ResetTrap(Entity trapPlatform);

        // Scaling platform specific methods
        Matrix3x3 GetScalingTransform(Entity scalingPlatform) const;
    };
}