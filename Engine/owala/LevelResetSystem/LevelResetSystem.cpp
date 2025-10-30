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

    // Constructor
    LevelResetSystem::LevelResetSystem(EntityStorage& ecs) : ecs_(ecs)
    {
        // Player Signature
        playerSig.set(ActiveTagType);
        playerSig.set(PlayerTagType);
        playerSig.set(PositionType);
        playerSig.set(VelocityType);
        playerSig.set(RotationFlagType);

        // Platform signatures
        trapPlatformSig.set(ActiveTagType);
        trapPlatformSig.set(TrapPlatformComponentType);
        trapPlatformSig.set(PositionType);
        trapPlatformSig.set(RotationFlagType);

        scalingPlatformSig.set(ActiveTagType);
        scalingPlatformSig.set(ScalingPlatformComponentType);
        scalingPlatformSig.set(PositionType);
        scalingPlatformSig.set(RotationFlagType);
    }

    void LevelResetSystem::RunReset() {
        // Reset global rotation (todo)

        // Reset entities
        EntityManager entityMgr(ecs_);
        for (const auto& [entity, signature] : entityMgr.GetEntitySignatures()) {
            // --- Player Reset ---
            if (CmpSignatures(signature, playerSig)) {
                ResetPosition(entityMgr, entity);
                ResetVelocity(entityMgr, entity);
                ResetRotation(entityMgr, entity);
                continue;
            }

            // --- Trap Platform Reset ---
            if (CmpSignatures(signature, trapPlatformSig)) {
                TrapPlatformComponent& platform = entityMgr.GetComponent<TrapPlatformComponent>(entity);
                ResetPosition(entityMgr, entity, Position(platform.originalPosition));
                platform.isTriggered = false;
                platform.hasFallen = false;
                continue;
            }

            // --- Scaling Platform Reset ---
            if (CmpSignatures(signature, scalingPlatformSig)) {
                ScalingPlatformComponent& platform = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
                platform.currentScaleX = {};
                platform.currentScaleY = {};
                platform.time = {};
                continue;
            }
        }
    }

    // --- Reset Helpers ---
    
    // Reset Position to given position, defaults to 0,0 if no Position provided
    void LevelResetSystem::ResetPosition(EntityManager& entityMgr, Entity entity, Position pos) {
        (entityMgr.GetComponent<Position>(entity)) = pos;
    }

    // Reset Velocity to given velocity, default to 0,0 if no Velocity provided
    void LevelResetSystem::ResetVelocity(EntityManager& entityMgr, Entity entity, Velocity vel) {
        (entityMgr.GetComponent<Velocity>(entity)) = vel;
    }
    
    // Reset RotationFlag component
    void LevelResetSystem::ResetRotation(EntityManager& entityMgr, Entity entity) {
        RotationFlag& rot = entityMgr.GetComponent<RotationFlag>(entity);
        rot.isRotating = false;
        rot.currentRotation = 0.0f;
        rot.targetRotation = 0.0f;
    }

    // --- Helper Functions --- 
    bool LevelResetSystem::CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch) {
        return ((sigToTest & sigToMatch) == sigToMatch);
    }

} // namespace Framework