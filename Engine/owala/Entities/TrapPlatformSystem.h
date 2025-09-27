/*****************************************************************//**
* \file   TrapPlatformSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"

namespace Framework {
    class TrapPlatformSystem : public ISystem {
    private:
        EntityStorage* ecs_;

    public:
        TrapPlatformSystem(EntityStorage& storage) : ecs_(&storage) {}

        void Initialize() override {}
        void Update(float dt) override;
        void Shutdown() override {}
    };
}