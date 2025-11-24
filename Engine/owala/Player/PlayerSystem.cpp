/*****************************************************************//**
 * \file   PlayerSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (50%)
 *
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

namespace Framework {
    void PlayerSystem::Initialize() {
        FileIOSystem::LoadPlayerStats(moveSpeed, jumpHeight);
    }

	void PlayerSystem::Update() {
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
                    OnPlayerLanded(entity, currentPos, std::abs(prevVelY));
                }

                // Update the component for the next frame
                state.previousVelocityY = currentVelY;

                // Horizontal Movement 
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

                // Animation State Handling
				AnimationState animState = AnimationState::IDLE; // Default to IDLE

				if (state.isGrounded) {
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


    void PlayerSystem::Shutdown() {
    }

	// Publish particle burst event when player lands
    void PlayerSystem::OnPlayerLanded(Entity player, const Vector2D& position, float impactSpeed) {

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

} // namespace Framework
