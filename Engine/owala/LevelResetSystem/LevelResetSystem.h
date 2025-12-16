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
#include "EntityStorage.h"
#include "EntityManager.h"
#include "TransformationSystem.h"
#include "Logger.h"

namespace Framework {
    class LevelResetSystem {
    private:
        // Reference to the EntityStorage to access entities and components
        EntityStorage& ecs_;
        TransformationSystem& transformationSys_;

        // Store the signatures required for quick checking (built in the constructor)
        ComponentSignature playerSig;
        ComponentSignature playerSpawnPtSig;

        // Platform signatures
        ComponentSignature basicPlatformSig;
        ComponentSignature trapPlatformSig;
        ComponentSignature scalingPlatformSig;

        // Enemy signatures
        ComponentSignature enemySig;

    public:
        // Constructor initializes the system with necessary references/masks
        LevelResetSystem(EntityStorage& ecs, TransformationSystem& transSys);

        // Reset current scene
        void RunReset();
        
        // Reset Player
        void ResetPlayer(EntityManager& entityMgr, Entity entity);
        
        // --- Reset Helpers ---
        // Reset Transform of a Entity, uses default transform if no Transform provided
        void ResetTransform(EntityManager& entityMgr, Entity entity, Transform transform = Transform());
        // Reset Velocity to given velocity, default to 0,0 if no Velocity provided
        void ResetVelocity(EntityManager& entityMgr, Entity entity, Velocity vel = Velocity());
        
        // Helper function to compare signatures
        bool CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch);

        Vector2D GetPlayerSpawnPt(EntityManager& entityMgr) const;
    };

} // namespace Framework

