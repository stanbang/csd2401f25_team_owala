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

        // Platform signatures
        ComponentSignature basicPlatformSig;
        ComponentSignature trapPlatformSig;
        ComponentSignature scalingPlatformSig;

    public:
        // Constructor initializes the system with necessary references/masks
        LevelResetSystem(EntityStorage& ecs, TransformationSystem& transSys);

        // Reset current scene
        void RunReset();
        
        // Helper function to compare signatures
        bool CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch);

        // --- Reset Helpers ---
        // Reset Position to given position, defaults to 0,0 if no Position provided
        void ResetPosition(EntityManager& entityMgr, Entity entity, Position pos = Position(Vector2D{}) );
        // Reset Velocity to given velocity, default to 0,0 if no Velocity provided
        void ResetVelocity(EntityManager& entityMgr, Entity entity, Velocity vel = Velocity());
        // Reset RotationFlag component
        void ResetRotation(EntityManager& entityMgr, Entity entity);
    };

} // namespace Framework

