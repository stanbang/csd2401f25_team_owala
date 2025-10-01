/*****************************************************************//**
* \file   TrapPlatformSystem.h
*
* \primary_author
*      Sitara Anil
* \secondary_author
*
*
* \brief    System that manages the update cycle for all trap platforms,
*           tracking their triggering and falling behavior each frame.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "EntityStorage.h"

namespace Framework {
    class TrapPlatformSystem {
    private:
        EntityStorage& ecs_;

    public:
        TrapPlatformSystem(EntityStorage& ecs) : ecs_(ecs) {}
        static void Update(EntityStorage& ecs, float dt);
    };
}