/*****************************************************************//**
 * \file   LevelResetSystem.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 * \brief Defines the LevelResetSystem class responsible for resetting the state of entities 
 *        (like players and platforms) to their initial configuration.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "Vector2D.h"

// Forward Declaration
namespace Framework {
    class EntityManager;
}

namespace Framework {
    class LevelResetSystem : public ISystem {
    private:

        // Store the signatures required for quick checking (built in the constructor)
        //ComponentSignature playerSig;
        ComponentSignature playerSpawnPtSig;

        // Platform signatures
        //ComponentSignature basicPlatformSig;
        //ComponentSignature trapPlatformSig;
        //ComponentSignature scalingPlatformSig;

        // Enemy signatures
        //ComponentSignature enemySig;

        // Text signature
        //ComponentSignature worldTextSig;

    public:
        void Initialize() override {}
        void Update() override {}
        void Shutdown() override {}

        LevelResetSystem();

        // Reset current scene
        void RunReset();
        
        // Reset Player
        //void ResetPlayer(EntityManager& entityMgr, Entity entity);
        
        // --- Reset Helpers ---
        // Reset Transform of a Entity, uses default transform if no Transform provided
        void ResetTransform(EntityManager& entityMgr, Entity entity, Transform transform = Transform());
        // Reset Velocity to given velocity, default to 0,0 if no Velocity provided
        //void ResetVelocity(EntityManager& entityMgr, Entity entity, Velocity vel = Velocity());
        
        // Helper function to compare signatures
        bool CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch);

        Vector2D GetPlayerSpawnPt(EntityManager& entityMgr) const;
    };

} // namespace Framework

