/*****************************************************************//**
* \file   TrapPlatform.h
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
#include "Entity.h"
#include "Vector2D.h"
#include <string>
#include <Components.h>

namespace Framework {
    struct TrapPlatformComponent;
    class EntityStorage;
    void updateTrapPlatforms(EntityStorage& entityStorage, float dt);
    void triggerTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform);
    void resetTrapPlatform(EntityStorage& entityStorage, Entity trapPlatform);
}