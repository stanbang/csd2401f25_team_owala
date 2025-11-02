/*****************************************************************//**
 * \file   LevelResetSystem.cpp
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

#include "LevelResetSystem.h"

namespace Framework {

    // Constructor
    LevelResetSystem::LevelResetSystem(EntityStorage& ecs, TransformationSystem& transSys) : ecs_(ecs), transformationSys_(transSys)
    {
        // Player Signature
        playerSig.set(ActiveTagType);
        playerSig.set(PlayerTagType);
        playerSig.set(PositionType);
        playerSig.set(VelocityType);
        playerSig.set(RotationFlagType);
        playerSig.set(SpawnPointType);

        // Platform signatures
        trapPlatformSig.set(ActiveTagType);
        trapPlatformSig.set(TrapPlatformComponentType);
        trapPlatformSig.set(PositionType);
        trapPlatformSig.set(RotationFlagType);

        scalingPlatformSig.set(ActiveTagType);
        scalingPlatformSig.set(ScalingPlatformComponentType);
        scalingPlatformSig.set(PositionType);
        scalingPlatformSig.set(RotationFlagType);
    
        basicPlatformSig.set(ActiveTagType);
        basicPlatformSig.set(PlatformTagType);
        basicPlatformSig.set(PositionType);
        basicPlatformSig.set(SpawnPointType);
    }

    void LevelResetSystem::RunReset() {
        // Reset global rotation
        Logger::Get().Info(DebugType::LevelResetSystem) << "Reset global rotation variables" << std::endl;
        transformationSys_.ResetGlobalRotation();

        // Reset entities
        EntityManager entityMgr(ecs_);
        for (const auto& [entity, signature] : entityMgr.GetEntitySignatures()) {
            // --- Player Reset ---
            if (CmpSignatures(signature, playerSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Player, Entity: " << entity << std::endl;
                SpawnPoint& spawnPt = entityMgr.GetComponent<SpawnPoint>(entity);
                ResetPosition(entityMgr, entity, Position(spawnPt.Pos));
                ResetVelocity(entityMgr, entity);
                ResetRotation(entityMgr, entity);
                continue;
            }

            // --- Trap Platform Reset ---
            if (CmpSignatures(signature, trapPlatformSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Trap Platform, Entity: " << entity << std::endl;
                TrapPlatformComponent& platform = entityMgr.GetComponent<TrapPlatformComponent>(entity);
                ResetPosition(entityMgr, entity, Position(platform.originalPosition));
                ResetRotation(entityMgr, entity);
                platform.isTriggered = false;
                platform.hasFallen = false;
                continue;
            }

            // --- Scaling Platform Reset ---
            if (CmpSignatures(signature, scalingPlatformSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Scaling Platform, Entity: " << entity << std::endl;
                ScalingPlatformComponent& platform = entityMgr.GetComponent<ScalingPlatformComponent>(entity);
                ResetRotation(entityMgr, entity);
                platform.currentScaleX = {};
                platform.currentScaleY = {};
                platform.time = {};
                continue;
            }

            // --- Basic Platform Reset ---
            if (CmpSignatures(signature, basicPlatformSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Basic Platform, Entity: " << entity << std::endl;
                // basic ground needs spawn point to store initial position bc rotation changes its position
                SpawnPoint& spawnPt = entityMgr.GetComponent<SpawnPoint>(entity);
                ResetPosition(entityMgr, entity, Position(spawnPt.Pos));
                ResetRotation(entityMgr, entity);
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
        rot.transformMatrix = {};
    }

    // --- Helper Functions --- 
    bool LevelResetSystem::CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch) {
        return ((sigToTest & sigToMatch) == sigToMatch);
    }

} // namespace Framework