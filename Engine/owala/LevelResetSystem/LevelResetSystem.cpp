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
#include "MessageBus.h"

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
        playerSig.set(SceneOwnedType);

        playerSpawnPtSig.set(ActiveTagType);
        playerSpawnPtSig.set(SpawnPointType);
        playerSpawnPtSig.set(SceneOwnedType);

        // Platform signatures
        trapPlatformSig.set(ActiveTagType);
        trapPlatformSig.set(TrapPlatformComponentType);
        trapPlatformSig.set(PositionType);
        trapPlatformSig.set(RotationFlagType);
        trapPlatformSig.set(SceneOwnedType);
    
        basicPlatformSig.set(ActiveTagType);
        basicPlatformSig.set(PlatformTagType);
        basicPlatformSig.set(PositionType);
        basicPlatformSig.set(SceneOwnedType);

        // Enemy signatures
        enemySig.set(EnemyTagType);
        enemySig.set(SceneOwnedType);
        enemySig.set(PositionType);
        enemySig.set(VelocityType);
        enemySig.set(SpawnPointType);
        enemySig.set(ActiveTagType);
        enemySig.set(SceneOwnedType);

        // Subscribe to ButtonClickedEvent for resetting player position
        MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {
            if (event.action == ButtonAction::RESETPLAYERPOS) {
                EntityManager entityMgr(ecs_);
                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
                    if (entityMgr.HasComponent(entity, PositionType)) {
                        ResetPosition(entityMgr, entity, GetPlayerSpawnPt(entityMgr));
                    }
                }
            }
        });
    }

    void LevelResetSystem::RunReset() {
        // Reset global rotation
        Logger::Get().Info(DebugType::LevelResetSystem) << "Reset global rotation variables" << std::endl;
        transformationSys_.ResetGlobalRotation();

        EntityManager entityMgr(ecs_);
        
        // Delete all particles
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ParticleComponentType)) {
			entityMgr.DestroyEntity(entity);
		}
        
        // Reset entities
        for (const auto& [entity, signature] : entityMgr.GetEntitySignatures()) {
            // --- Player Reset ---
            if (CmpSignatures(signature, playerSig)) {
                ResetPlayer(entityMgr, entity);
                continue;
            }

            // --- Trap Platform Reset ---
            if (CmpSignatures(signature, trapPlatformSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Trap Platform, Entity: " << entity << std::endl;
                TrapPlatformComponent& platform = entityMgr.GetComponent<TrapPlatformComponent>(entity);
                ResetPosition(entityMgr, entity, Position(platform.originalPosition));
                ResetRotation(entityMgr, entity);
                if (entityMgr.HasComponent(entity, RigidBodyType)) {
                    entityMgr.GetComponent<RigidBody>(entity).SetMass(0.0f);
                }

                if (entityMgr.HasComponent(entity, VelocityType)) {
                    entityMgr.GetComponent<Velocity>(entity).Vel = Vector2D::Zero();
                }
                platform.isTriggered = false;
                platform.hasFallen = false;
                continue;
            }

            // --- Basic Platform Reset ---
            if (CmpSignatures(signature, basicPlatformSig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Basic Platform, Entity: " << entity << std::endl;
                Position originalPos{ TransformationSystem::GetOriginalPos(entityMgr, entity) };
                ResetPosition(entityMgr, entity, originalPos);
                ResetRotation(entityMgr, entity);
                continue;
            }

            // --- Enemy Reset ---
            if (CmpSignatures(signature, enemySig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Enemy, Entity: " << entity << std::endl;
                Position originalPos{ entityMgr.GetComponent<SpawnPoint>(entity).Pos};
                ResetPosition(entityMgr, entity, originalPos);
                ResetVelocity(entityMgr, entity);
                //if (entityMgr.HasComponent(entity, BirdAIComponentType)) {
                //    BirdAIComponent& birdAI = entityMgr.GetComponent<BirdAIComponent>(entity);
                //}

                continue;
            }
        }
    }

    // --- Reset Helpers ---
    // Reset Player
    void LevelResetSystem::ResetPlayer(EntityManager& entityMgr, Entity entity) {
        Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Player, Entity: " << entity << std::endl;
        ResetPosition(entityMgr, entity, GetPlayerSpawnPt(entityMgr));
        ResetVelocity(entityMgr, entity);
        ResetRotation(entityMgr, entity);

        // Reset animation
        if (entityMgr.HasComponent(entity, AnimationComponentType)) {
            AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);
            anim.currentState = AnimationState::IDLE;
            anim.currentFrame = 0;
            anim.elapsedTime = 0.0f;
            anim.isPlaying = true;
        }
        if (entityMgr.HasComponent(entity, PlayerStateType)) {
            PlayerState& playerState = entityMgr.GetComponent<PlayerState>(entity);
            playerState.isDead = false;
            Logger::Get().Info(DebugType::LevelResetSystem) << "Reset player state - isDead: false" << std::endl;
        }
        if (entityMgr.HasComponent(entity, RigidBodyType)) {
            RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
            rb.SetMass(1.0f);
            rb.isAffectedByGravity = true;
        }
    
    }
    
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

    Position LevelResetSystem::GetPlayerSpawnPt(EntityManager& entityMgr) const {
        // Get player spawn point entity (should only have one in the scene)
        Entity spawnPtEntity = 0;
        for (const auto& [entity, signature] : entityMgr.GetEntitySignatures()) {
            if (signature == playerSpawnPtSig) { // == for strict matching. should ONLY have these components
                spawnPtEntity = entity;
                break;
            }
        }

        if (spawnPtEntity > 0) {
            SpawnPoint& spawnPt = entityMgr.GetComponent<SpawnPoint>(spawnPtEntity);
            return Position(spawnPt.Pos);
        }
        else {
            Logger::Get().Error(DebugType::LevelResetSystem) << "Player spawn point not found!" << std::endl;
        }

        return Position(Vector2D{});
    }

} // namespace Framework