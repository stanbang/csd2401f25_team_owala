/*****************************************************************//**
 * \file   LevelResetSystem.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 * \brief
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "EntityStorage.h"
#include "EntityManager.h"

namespace Framework {
    class LevelResetSystem {
    private:
        // Reference to the EntityStorage to access entities and components
        EntityStorage& ecs_;

        // Store the signatures required for quick checking (built in the constructor)
        ComponentSignature trapPlatformSig;
        ComponentSignature playerSig;

    public:
        // Constructor initializes the system with necessary references/masks
        LevelResetSystem(EntityStorage& ecs) : ecs_(ecs)
        {
            trapPlatformSig.set(TrapPlatformComponentType);
            trapPlatformSig.set(PositionType);

            playerSig.set(PlayerTagType);
            playerSig.set(PositionType);
            playerSig.set(VelocityType);
        }

        void RunReset();
    };

} // namespace Framework

