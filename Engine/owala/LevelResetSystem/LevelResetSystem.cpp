/*****************************************************************//**
 * \file	LevelResetSystem.cpp
 * \brief	System for resetting levels
 *
 *
 * \author	Yar Xuan
 * \date	29 October 2025
 *********************************************************************/

#include "LevelResetSystem.h"

namespace Framework {
    void LevelResetSystem::RunReset() {
        EntityManager entityMgr(ecs_);

        for (const auto& [entity, signature] : entityMgr.GetEntitySignatures()) {

            // --- Trap Platform Reset ---
            if ((signature & trapPlatformSig) == trapPlatformSig) {
                Position& pos = entityMgr.GetComponent<Position>(entity);
                TrapPlatformComponent& platform = entityMgr.GetComponent<TrapPlatformComponent>(entity);
                pos.Pos = platform.originalPosition;
                platform.isTriggered = false;
                platform.hasFallen = false;
            }

            // --- Player Reset ---
            if ((signature & playerSig) == playerSig) {
                Position& pos = entityMgr.GetComponent<Position>(entity);
                pos.Pos = Vector2D(0.0f, 0.0f);

                Velocity& velo = entityMgr.GetComponent<Velocity>(entity);
                velo.Vel = Vector2D(0.0f, 0.0f);
            }
        }
    }

} // namespace Framework