/*****************************************************************//**
 * \file   PlayerSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (45%)
 *      Sitara Anil (5%)
 * \brief	Contains Implementations of a Player System that
 *			updates the player entity's components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.   
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "EntityStorage.h"
#include "PlayerSystem.h"
#include <iostream>
#include "GameManager.h"
#include "InputSystem.h" // Include for Input System
#include "PhysicsSystem.h"
#include "FileUtility.h" // Include to write player data on Shutdown
#include "EntityManager.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include "ParticleEffectManager.h"

namespace Framework {
    void PlayerSystem::Initialize() {
        FileIOSystem::LoadPlayerStats(moveSpeed, jumpHeight);
		ParticleEffectManager::GetInstance().LoadEffectPresets("Data/ParticleEffects/particleEffects.json");
    }

	void PlayerSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        PhysicsSystem& physics_ = *gm.GetSystems().physics_;
        if (physics_.IsPaused()) {return;}
        InputSystem& input_ = *gm.GetSystems().input_;
        EntityManager entityMgr(gm.GetECS()); // To access entity creation/destruction if needed

        UpdateInvulnerability();
        CheckPlayerDeathFromTraps();
        CheckPlayerDeathFromEnemies();
        HandlePlayerDeath();
        CheckEnemyDeathFromTraps();

        // calculate jump vel based on height and gravity -  v = sqrt(2 * g * h).
        const float gravity = std::abs(physics_.gravity_.y);
        const float jumpVelocity = std::sqrt(2.0f * gravity * jumpHeight);

		// Loop through each entity that is in the playerTag map
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            // Check if Player Entity is active in the scene. Handle player controller if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;


            // We need a RigidBody, Velocity, and PlayerState to control the player
            if (entityMgr.HasComponent(entity, VelocityType) && entityMgr.HasComponent(entity, PlayerStateType) && entityMgr.HasComponent(entity, AnimationComponentType)) {
                Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);

                // Get Position
                Vector2D currentPos = { 0,0 };
                if (entityMgr.HasComponent(entity, TransformType)) {
                    currentPos = entityMgr.GetComponent<Transform>(entity).position;
                }

				// Get previous and current vertical velocities for landing detection
                float currentVelY = vel.Vel.y;
                float prevVelY = state.previousVelocityY; // Get from Component
                float stopThreshold = 0.01f;

                // Logic: 
                // 1. We were falling fast (prev < 0)
                // 2. We stopped vertically (current ~ 0)
                // 3. We are touching the ground (isGrounded)
                if (prevVelY < -stopThreshold &&
                    std::abs(currentVelY) < stopThreshold &&
                    state.isGrounded)
                {
                    // Pass the impact speed (absolute value of previous fall speed)
                    OnPlayerLanded(entity, currentPos);
                }

                // Update the component for the next frame
                state.previousVelocityY = currentVelY;

                // Horizontal Movement 
                if (!state.isDead) {
                    float targetVelX = 0.0f;
                    if (input_.KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
                    if (input_.KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
                    vel.Vel.x = targetVelX;

				    // Apply Jumping
                    if (state.isGrounded && (input_.KeyPressed(InputSystem::Key::W) || input_.KeyPressed(InputSystem::Key::Space))) {
                        vel.Vel.y = jumpVelocity; // Apply the calculated upward velocity

                        float jumpVolume = 0.3f;
                        //messageBus
                        MessageBus::Instance().Publish(PlayerJumpedEvent{ entity, jumpVolume}); //empty cause currently theres nothign to pass and just for testing
                    }

				    // Sets the direction the player is facing based on movement
                    if (targetVelX > 0.01f) {
                        state.isFacingRight = true;
                    }
                    else if (targetVelX < -0.01f) {
                        state.isFacingRight = false;
                    }
                }
                else {
                    vel.Vel.x = 0.0f;
                }
                if (state.isDead) {
                    vel.Vel.x = 0.0f;
                    if (state.isGrounded) {
                        if (entityMgr.HasComponent(entity, RigidBodyType)) {
                            RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
                            rb.InverseMass = 0.0f;
                        }
                        vel.Vel.y = 0.0f;
                    }
                }
                // Animation State Handling
                AnimationState animState = AnimationState::IDLE; // Default to IDLE

                // player death state
                if (state.isDead) {
                    animState = AnimationState::DEATH;
                }
                else if (state.isGrounded) {
                    if (std::abs(vel.Vel.x) > 0.01f) {
                        animState = AnimationState::RUNNING; //If player is on ground and moving, set to running
                    }
                    else {
                        animState = AnimationState::IDLE; //If player is on ground and not moving, set to idle
                    }
                }
                else {
                    if (vel.Vel.y > 0.01f) {
                        animState = AnimationState::JUMPING; //If player is in air and moving upwards, set to jumping
                    }
                    else if (vel.Vel.y <= 0.0f) {
                        animState = AnimationState::FALLING; //If player is in air, at the point he is beginning to fall down or moving downwards, set to falling
                    }
                }

                if (state.isInvulnerable) {
                        switch (animState) {
                        case AnimationState::IDLE:
                            animState = AnimationState::INVULNERABLE_IDLE;
                            break;
                        case AnimationState::RUNNING:
                            animState = AnimationState::INVULNERABLE_RUNNING;
                            break;
                        case AnimationState::JUMPING:
                            animState = AnimationState::INVULNERABLE_JUMPING;
                            break;
                        case AnimationState::FALLING:
                            animState = AnimationState::INVULNERABLE_FALLING;
                            break;
                        default:
                            break;
                        }
                }
                animationComponent.currentState = animState;
            }
        }
    }

    void PlayerSystem::UpdateInvulnerability() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
            if (!entityMgr.HasComponent(entity, PlayerStateType)) continue;

            PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);

            if (state.isInvulnerable) {
                state.invulnerabilityTimer += Framework::Time::DeltaTime();

                if (state.invulnerabilityTimer >= INVULNERABILITY_DURATION) {
                    // End invulnerability
                    state.isInvulnerable = false;
                    state.invulnerabilityTimer = 0.0f;

                    Framework::Logger::Get().Info(DebugType::PlayerSystem)  << "Player invulnerability ended. Lives remaining: " << state.currentLives << std::endl;
                }
            }
        }
    }


    // ========== PLAYER DEATH LOGIC ==========

    void PlayerSystem::CheckPlayerDeathFromTraps() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        Entity playerEntity = 0;
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (entityMgr.HasComponent(entity, LoadedTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead || playerState.isInvulnerable) return;

        for (const Entity& trapEntity : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
            std::string trapName = entityMgr.GetProperties(trapEntity).name;
            if (trapName.find("MovableBox") != std::string::npos) continue;
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

            if (!trapComp.isTriggered) continue;

            const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& trapPos = entityMgr.GetComponent<Transform>(trapEntity).position;
            const Vector2D& trapHalfSize = entityMgr.GetComponent<AABBCollider>(trapEntity).HalfSize;

            AABB_Bounds playerBounds = {
                {playerPos.x - playerHalfSize.x, playerPos.y - playerHalfSize.y},
                {playerPos.x + playerHalfSize.x, playerPos.y + playerHalfSize.y}
            };

            float trapRotation = 0.0f;
            if (entityMgr.HasComponent(trapEntity, RotationFlagType)) {
                trapRotation = entityMgr.GetComponent<RotationFlag>(trapEntity).currentRotation;
            }
            AABB_Bounds trapBounds = PhysicsSystem::GetRotatedAABB(trapPos, trapHalfSize, trapRotation);

            if (CollisionIntersection_RectRect_Static(playerBounds, trapBounds)) {
                float trapBottom = trapBounds.min.y;
                float playerTop = playerBounds.max.y;

                bool shouldDamage = (trapBottom >= playerTop);

                if (shouldDamage) {
                    DamagePlayer(playerEntity, "falling trap platform");
                    break;
                }
            }
        }
    }

    void PlayerSystem::CheckPlayerDeathFromEnemies() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        Entity playerEntity{};
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (entityMgr.HasComponent(entity, LoadedTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead || playerState.isInvulnerable) return;

        for (const Entity& enemy : entityMgr.GetAllEntitiesWithComponent(true, EnemyTagType)) {
            if (!entityMgr.HasComponent(enemy, TransformType) ||
                !entityMgr.HasComponent(enemy, AABBColliderType) ||
                !entityMgr.HasComponent(playerEntity, TransformType) ||
                !entityMgr.HasComponent(playerEntity, AABBColliderType)) {
                continue;
            }

            const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemy).position;
            const Vector2D& enemyHalfSize = entityMgr.GetComponent<AABBCollider>(enemy).HalfSize;

            AABB_Bounds playerBounds = {
                {playerPos.x - playerHalfSize.x, playerPos.y - playerHalfSize.y},
                {playerPos.x + playerHalfSize.x, playerPos.y + playerHalfSize.y}
            };

            AABB_Bounds enemyBounds = {
                {enemyPos.x - enemyHalfSize.x, enemyPos.y - enemyHalfSize.y},
                {enemyPos.x + enemyHalfSize.x, enemyPos.y + enemyHalfSize.y}
            };

            if (CollisionIntersection_RectRect_Static(playerBounds, enemyBounds)) {
                std::string enemyType = "enemy";
                if (entityMgr.HasComponent(enemy, FSMComponentType)) {
                    enemyType = "bird enemy";
                }

                DamagePlayer(playerEntity, enemyType);
                break;
            }
        }
    }

    void PlayerSystem::DamagePlayer(Entity playerEntity, const std::string& damageSource) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, PlayerStateType)) {
            return;
        }

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        Framework::Logger::Get().Info(DebugType::PlayerSystem)<< "Before damage - Lives: " << playerState.currentLives
            << " Invulnerable: " << playerState.isInvulnerable
            << " Dead: " << playerState.isDead << std::endl;

        // Already invulnerable or dead
        if (playerState.isInvulnerable || playerState.isDead) {
            return;
        }

        // Lose a life
        playerState.currentLives--;

        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player hit by " << damageSource << "! Lives remaining: " << playerState.currentLives << std::endl;

        // Check if player is out of lives
        if (playerState.currentLives <= 0) {
            KillPlayer(playerEntity, damageSource);
        }
        else {
            // Activate invulnerability
            playerState.isInvulnerable = true;
            playerState.invulnerabilityTimer = 0.0f;

            MessageBus::Instance().Publish(PlayerDamagedEvent{ playerEntity, playerState.currentLives });
        }
    }


    bool PlayerSystem::IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, TransformType) ||
            !entityMgr.HasComponent(enemyEntity, TransformType)) {
            return false;
        }

        const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
        const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemyEntity).position;

        float distance = playerPos.Distance(enemyPos);
        return distance <= attackRadius;
    }

    void PlayerSystem::KillPlayer(Entity playerEntity, const std::string& killerType) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, PlayerStateType)) {
            return;
        }

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead) return;

        playerState.isDead = true;
        playerState.currentLives = 0;

        /*if (entityMgr.HasComponent(playerEntity, VelocityType)) {
            entityMgr.GetComponent<Velocity>(playerEntity).Vel = Vector2D::Zero();
        }
        if (entityMgr.HasComponent(playerEntity, RigidBodyType)) {
            entityMgr.GetComponent<RigidBody>(playerEntity).SetMass(0.0f);
        }*/
      

        if (entityMgr.HasComponent(playerEntity, VelocityType)) {
            Velocity& vel = entityMgr.GetComponent<Velocity>(playerEntity);
            vel.Vel.x = 0.0f;
        }
        if (entityMgr.HasComponent(playerEntity, AnimationComponentType)) {
            entityMgr.GetComponent<AnimationComponent>(playerEntity).currentState = AnimationState::DEATH;
        }

        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player killed by " << killerType << "!" << std::endl;
    }

    void PlayerSystem::CheckEnemyDeathFromTraps() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& enemyEntity : entityMgr.GetAllEntitiesWithComponent(true, FSMComponentType)) {
            if (!entityMgr.HasComponent(enemyEntity, EnemyTagType) || !entityMgr.HasComponent(enemyEntity, TransformType) ||
                !entityMgr.HasComponent(enemyEntity, AABBColliderType)) {
                continue;
            }

            FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(enemyEntity);

            if (fsmComp.fsm.GetCurrentState() == "Death") continue;

            // Check  all trap platforms
            for (const Entity& trapEntity : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
                std::string trapName = entityMgr.GetProperties(trapEntity).name;
                if (trapName.find("MovableBox") != std::string::npos) continue;
                TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

                if (!trapComp.isTriggered) continue;

                const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemyEntity).position;
                const Vector2D& enemyHalfSize = entityMgr.GetComponent<AABBCollider>(enemyEntity).HalfSize;
                const Vector2D& trapPos = entityMgr.GetComponent<Transform>(trapEntity).position;
                const Vector2D& trapHalfSize = entityMgr.GetComponent<AABBCollider>(trapEntity).HalfSize;

                // Enemy bounds
                AABB_Bounds enemyBounds = {
                    {enemyPos.x - enemyHalfSize.x, enemyPos.y - enemyHalfSize.y},
                    {enemyPos.x + enemyHalfSize.x, enemyPos.y + enemyHalfSize.y}
                };

                float trapRotation = 0.0f;
                if (entityMgr.HasComponent(trapEntity, RotationFlagType)) {
                    trapRotation = entityMgr.GetComponent<RotationFlag>(trapEntity).currentRotation;
                }
                AABB_Bounds trapBounds = PhysicsSystem::GetRotatedAABB(trapPos, trapHalfSize, trapRotation);

                if (CollisionIntersection_RectRect_Static(enemyBounds, trapBounds)) {
                    float trapBottom = trapBounds.min.y;
                    float enemyTop = enemyBounds.max.y;

                    bool shouldKill = (trapBottom >= enemyTop);

                    if (shouldKill) {
                        TriggerEnemyDeath(enemyEntity, "falling trap platform");
                        break;
                    }
                }
            }
        }
    }

    void PlayerSystem::TriggerEnemyDeath(Entity enemyEntity, const std::string& killerType) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(enemyEntity, FSMComponentType)) {
            return;
        }

        FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(enemyEntity);

        // Transition to death state
        fsmComp.fsm.ChangeState(enemyEntity, GameManager::GetInstance().GetECS(), "Death");

        Framework::Logger::Get().Info(DebugType::PlayerSystem)  << "Enemy killed by " << killerType << "!" << std::endl;
    }

    void PlayerSystem::Shutdown() {
    }

	// Publish particle burst event when player lands
    void PlayerSystem::OnPlayerLanded(Entity player, const Vector2D& position) {
        MessageBus::Instance().Publish(PlayerLandEvent());
        // Publish VFX Trigger Event for Player Landed, with position override true, and the player's position to tell the event where to spawn the VFX
        MessageBus::Instance().Publish(VFXTriggerEvent{ "PlayerLandedEvent", true, position });
    }

    void PlayerSystem::HandlePlayerDeath() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

            if (entityMgr.HasComponent(entity, PlayerStateType)) {
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);

                if (state.isDead && entityMgr.HasComponent(entity, AnimationComponentType)) {
                    AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);

                    if (anim.currentState == AnimationState::DEATH && !anim.isPlaying) {
                        MessageBus::Instance().Publish(PlayerDeathEvent{ entity, true }); // animation finished
                        /*Commented out because we dont want the player death animation to be looping as the curtains are closing*/
						//state.isDead = false; // Reset death state after handling
                    }
                }
            }
        }
    }

} // namespace Framework
