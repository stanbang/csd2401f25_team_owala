/*****************************************************************//**
* \file   ScalingPlatformSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief    System that manages the update cycle for all scaling platforms,
*           handling their animation and transformation updates each frame.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"

namespace Framework {
    class ScalingPlatformSystem : public ISystem {
    private:
        EntityStorage* ecs_;

    public:
        ScalingPlatformSystem(EntityStorage& storage) : ecs_(&storage) {}

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}
    };
}