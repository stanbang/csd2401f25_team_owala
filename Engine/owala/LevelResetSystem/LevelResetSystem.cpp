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
        playerSig.set(TransformType);
        playerSig.set(VelocityType);
        playerSig.set(RotationFlagType);
        playerSig.set(SceneOwnedType);

        playerSpawnPtSig.set(ActiveTagType);
        playerSpawnPtSig.set(SpawnPointType);
        playerSpawnPtSig.set(SceneOwnedType);

        // Platform signatures
        trapPlatformSig.set(ActiveTagType);
        trapPlatformSig.set(TrapPlatformComponentType);
        trapPlatformSig.set(TransformType);
        trapPlatformSig.set(RotationFlagType);
        trapPlatformSig.set(SceneOwnedType);
    
        basicPlatformSig.set(ActiveTagType);
        basicPlatformSig.set(PlatformTagType);
        basicPlatformSig.set(TransformType);
        basicPlatformSig.set(SceneOwnedType);

        // Enemy signatures
        enemySig.set(EnemyTagType);
        enemySig.set(SceneOwnedType);
        enemySig.set(TransformType);
        enemySig.set(VelocityType);
        enemySig.set(SpawnPointType);
        enemySig.set(ActiveTagType);
        enemySig.set(SceneOwnedType);

        // Text signature
        worldTextSig.set(TextComponentType);
        worldTextSig.set(TransformType);
        worldTextSig.set(RotationFlagType);
        worldTextSig.set(ActiveTagType);
        worldTextSig.set(SceneOwnedType);

        // Subscribe to ButtonClickedEvent for resetting player position
        MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {
            if (event.action == ButtonAction::RESETPLAYERPOS) {
                EntityManager entityMgr(ecs_);
                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
                    if (entityMgr.HasComponent(entity, TransformType)) {
                        ResetTransform(entityMgr, entity, Transform(GetPlayerSpawnPt(entityMgr)));
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
                ResetTransform(entityMgr, entity, Transform(platform.originalPosition));
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
                Vector2D originalPos = TransformationSystem::GetOriginalPos(entityMgr, entity);
                ResetTransform(entityMgr, entity, Transform{ originalPos });
                continue;
            }

            // --- Enemy Reset ---
            if (CmpSignatures(signature, enemySig)) {
                Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Enemy, Entity: " << entity << std::endl;
                Vector2D originalPos = entityMgr.GetComponent<SpawnPoint>(entity).Pos;
                ResetTransform(entityMgr, entity, Transform{ originalPos });
                ResetVelocity(entityMgr, entity);

                continue;
            }

            // --- Portal Reset ---
            if (entityMgr.GetProperties(entity).name == "Portal") {
                Vector2D originalPos = TransformationSystem::GetOriginalPos(entityMgr, entity);
                ResetTransform(entityMgr, entity, Transform{ originalPos });
                continue;
            }

            // --- World Text Reset ---
            if (CmpSignatures(signature, worldTextSig)) {
                Vector2D originalPos = TransformationSystem::GetOriginalPos(entityMgr, entity);
                ResetTransform(entityMgr, entity, Transform{ originalPos });
                continue;
            }
        }
    }

    // --- Reset Helpers ---
    // Reset Player
    void LevelResetSystem::ResetPlayer(EntityManager& entityMgr, Entity entity) {
        Logger::Get().Info(DebugType::LevelResetSystem) << "Resetting Player, Entity: " << entity << std::endl;
        ResetTransform(entityMgr, entity, Transform(GetPlayerSpawnPt(entityMgr)));
        ResetVelocity(entityMgr, entity);

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

    // Reset Transform of a Entity, uses default transform if no Transform provided
    void LevelResetSystem::ResetTransform(EntityManager& entityMgr, Entity entity, Transform transform) {
        entityMgr.GetComponent<Transform>(entity) = transform;

        if (entityMgr.HasComponent(entity, RotationFlagType)) {
            RotationFlag& rot = entityMgr.GetComponent<RotationFlag>(entity);
            rot.isRotating = false;
            rot.currentRotation = 0.0f;
            rot.targetRotation = 0.0f;
            rot.transformMatrix = {};
        }
    }

    // Reset Velocity to given velocity, default to 0,0 if no Velocity provided
    void LevelResetSystem::ResetVelocity(EntityManager& entityMgr, Entity entity, Velocity vel) {
        (entityMgr.GetComponent<Velocity>(entity)) = vel;
    }

    // --- Helper Functions --- 
    bool LevelResetSystem::CmpSignatures(ComponentSignature sigToTest, ComponentSignature sigToMatch) {
        return ((sigToTest & sigToMatch) == sigToMatch);
    }

    Vector2D LevelResetSystem::GetPlayerSpawnPt(EntityManager& entityMgr) const {
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
            return spawnPt.Pos;
        }
        else {
            Logger::Get().Error(DebugType::LevelResetSystem) << "Player spawn point not found!" << std::endl;
        }

        return Vector2D{};
    }

} // namespace Framework