/*********************************************************************//**
* \file   TrapPlatformSystem.h
*
* \brief
* Game-layer system responsible for trap-platform landing logic.
* Subscribes to the engine's generic EntityLandedEvent and applies
* trap-specific state changes (hasFallen reset, sound, land event).
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include "ISystem.h"

namespace Framework {

    class TrapPlatformSystem : public ISystem {
    public:
        void Initialize() override;
        void Update() override;
        void Shutdown() override {}
    };

} // namespace Framework
