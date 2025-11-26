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
#include "EntityStorage.h"
#include "PlayerSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes
#include "InputSystem.h" // Include for Input System
#include "PhysicsSystem.h"
#include "FileUtility.h" // Include to write player data on Shutdown
#include "EntityManager.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include <cmath>
#include <OwalaTime.h>
#include "TransformationSystem.h"

namespace Framework {
    void PlayerSystem::Initialize() {
        FileIOSystem::LoadPlayerStats(moveSpeed, jumpHeight);
    }

	void PlayerSystem::Update() {
        CheckPlayerDeathFromTraps();
        CheckPlayerDeathFromEnemies();
        HandlePlayerDeath();
        // calculate jump vel based on height and gravity -  v = sqrt(2 * g * h).
        const float gravity = std::abs(physics_->gravity_.y);
        const float jumpVelocity = std::sqrt(2.0f * gravity * jumpHeight);

		EntityManager entityMgr(*entityStorage_); // To access entity creation/destruction if needed
		// Loop through each entity that is in the playerTag map
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            // Check if Player Entity is active in the scene. Handle player controller if active.
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;


            // We need a RigidBody, Velocity, and PlayerState to control the player
            if (entityMgr.HasComponent(entity, VelocityType) && entityMgr.HasComponent(entity, PlayerStateType) && entityMgr.HasComponent(entity, AnimationComponentType)) {
                Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);

                // Get Position
                Vector2D currentPos = { 0,0 };
                if (entityMgr.HasComponent(entity, PositionType)) {
                    currentPos = entityMgr.GetComponent<Position>(entity).Pos;
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
                    if (input_->KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
                    if (input_->KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
                    vel.Vel.x = targetVelX;

				    // Apply Jumping
                    if (state.isGrounded && (input_->KeyPressed(InputSystem::Key::W) || input_->KeyPressed(InputSystem::Key::Space))) {
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

                // Animation State Handling
				AnimationState animState = AnimationState::IDLE; // Default to IDLE
                
                // player death state
                if (state.isDead) { 
                    animState = AnimationState::DEATH;                
                }
                else if (state.isGrounded) {
                    if (std::abs(vel.Vel.x) > 0.01f) { animState = AnimationState::RUNNING; } //If player is on ground and moving, set to running
                    else { animState = AnimationState::IDLE; } //If player is on ground and not moving, set to idle
				}
				else {
                    if (vel.Vel.y > 0.01f) { animState = AnimationState::JUMPING; } //If player is in air and moving upwards, set to jumping
                    else if (vel.Vel.y <= 0.0f) { animState = AnimationState::FALLING; } //If player is in air, at the point he is beginning to fall down or moving downwards, set to falling
                }
               
                animationComponent.currentState = animState;
            }
        }
    }



    // ========== PLAYER DEATH LOGIC ==========

    void PlayerSystem::CheckPlayerDeathFromTraps() {
        EntityManager entityMgr(*entityStorage_);

        // Get player
        Entity playerEntity = 0;
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (entityMgr.HasComponent(entity, ActiveTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead) return;

        for (const Entity& trapEntity : entityMgr.GetAllEntitiesWithComponent(TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

            if (!trapComp.isTriggered) continue;

            const Vector2D& playerPos = entityMgr.GetComponent<Position>(playerEntity).Pos;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& trapPos = entityMgr.GetComponent<Position>(trapEntity).Pos;
            const Vector2D& trapHalfSize = entityMgr.GetComponent<AABBCollider>(trapEntity).HalfSize;

            // Player bounds (player doesn't rotate)
            AABB_Bounds playerBounds = {
                {playerPos.x - playerHalfSize.x, playerPos.y - playerHalfSize.y},
                {playerPos.x + playerHalfSize.x, playerPos.y + playerHalfSize.y}
            };

            // Get trap's rotation and compute rotated AABB
            float trapRotation = 0.0f;
            if (entityMgr.HasComponent(trapEntity, RotationFlagType)) {
                trapRotation = entityMgr.GetComponent<RotationFlag>(trapEntity).currentRotation;
            }
            AABB_Bounds trapBounds = PhysicsSystem::GetRotatedAABB(trapPos, trapHalfSize, trapRotation);

            if (CollisionIntersection_RectRect_Static(playerBounds, trapBounds)) {
                float trapBottom = trapBounds.min.y;
                float playerTop = playerBounds.max.y;

                // Trap kills if its bottom edge is at or above player's top
                bool shouldKill = (trapBottom >= playerTop);

                if (shouldKill) {
                    KillPlayer(playerEntity, "falling trap platform");
                    break;
                }
            }
        }
    }

    void PlayerSystem::CheckPlayerDeathFromEnemies() {
        EntityManager entityMgr(*entityStorage_);

        // Get player
        Entity playerEntity{};
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (entityMgr.HasComponent(entity, ActiveTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead) return;

        // Check all enemies for collision with player
        for (const Entity& enemy : entityMgr.GetAllEntitiesWithComponent(EnemyTagType)) {
            if (!entityMgr.HasComponent(enemy, PositionType) ||
                !entityMgr.HasComponent(enemy, AABBColliderType) ||
                !entityMgr.HasComponent(playerEntity, PositionType) ||
                !entityMgr.HasComponent(playerEntity, AABBColliderType)) {
                continue;
            }

            const Vector2D& playerPos = entityMgr.GetComponent<Position>(playerEntity).Pos;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& enemyPos = entityMgr.GetComponent<Position>(enemy).Pos;
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
                if (entityMgr.HasComponent(enemy, BirdAIComponentType)) {
                    enemyType = "bird enemy";
                }
                else if (entityMgr.HasComponent(enemy, SkeletonAIComponentType)) {
                    enemyType = "skeleton enemy";
                }

                KillPlayer(playerEntity, enemyType);
                break;
            }
        }
    }

    bool PlayerSystem::IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius) {
        EntityManager entityMgr(*entityStorage_);

        if (!entityMgr.HasComponent(playerEntity, PositionType) ||
            !entityMgr.HasComponent(enemyEntity, PositionType)) {
            return false;
        }

        const Vector2D& playerPos = entityMgr.GetComponent<Position>(playerEntity).Pos;
        const Vector2D& enemyPos = entityMgr.GetComponent<Position>(enemyEntity).Pos;

        float distance = playerPos.Distance(enemyPos);
        return distance <= attackRadius;
    }

    void PlayerSystem::KillPlayer(Entity playerEntity, const std::string& killerType) {
        EntityManager entityMgr(*entityStorage_);

        if (!entityMgr.HasComponent(playerEntity, PlayerStateType)) {
            return;
        }

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead) return; // Already dead

        playerState.isDead = true;

        if (entityMgr.HasComponent(playerEntity, VelocityType)) {
            entityMgr.GetComponent<Velocity>(playerEntity).Vel = Vector2D::Zero();
        }
        if (entityMgr.HasComponent(playerEntity, RigidBodyType)) {
            entityMgr.GetComponent<RigidBody>(playerEntity).SetMass(0.0f);
        }

        // Set death animation
        if (entityMgr.HasComponent(playerEntity, AnimationComponentType)) {
            entityMgr.GetComponent<AnimationComponent>(playerEntity).currentState = AnimationState::DEATH;
        }

        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player killed by " << killerType << "!" << std::endl;
    }

    void PlayerSystem::Shutdown() {
    }

	// Publish particle burst event when player lands
    void PlayerSystem::OnPlayerLanded(Entity player, const Vector2D& position) {

        EntityManager entityMgr(*entityStorage_);
        Vector2D playerHalfSize = { 0.1f, 0.1f };

        if (entityMgr.HasComponent(player, AABBColliderType)) {
            playerHalfSize = entityMgr.GetComponent<AABBCollider>(player).HalfSize;
        }

        ParticleBurstEvent burst;
        burst.position = position;  // Player center
        burst.offset = { 0.0f, 0.0f };  // Shift to feet
        burst.textureKey = "SB_PlayerLand_VFX_Spritesheet.png";
        burst.isAnimated = true;
        burst.style = EmissionStyle::FROM_POINT;
        burst.isScreenSpace = false;
        burst.lifetime = 0.7f;
        burst.count = 1;
        burst.speed = 0.0f;
        burst.size = { 100, 100 };

        MessageBus::Instance().Publish(burst);
    }

    void PlayerSystem::HandlePlayerDeath() {
        EntityManager entityMgr(*entityStorage_);

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

            if (entityMgr.HasComponent(entity, PlayerStateType)) {
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);

                if (state.isDead && entityMgr.HasComponent(entity, AnimationComponentType)) {
                    AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);

                    if (anim.currentState == AnimationState::DEATH && !anim.isPlaying) {
                        MessageBus::Instance().Publish(PlayerDeathEvent{ entity, true }); // animation finished
						state.isDead = false; // Reset death state after handling
                    }
                }
            }
        }
    }

} // namespace Framework
