/*****************************************************************//**
 * \file   PlayerSystem.cpp
 * \brief  
 * 
 * 
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.h"
#include "PlayerSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes
#include "InputSystem.h" // Include for Input System
#include "PhysicsSystem.h"
#include "FileUtility.h" // Include to write player data on Shutdown


namespace Framework {

	void PlayerSystem::Update(float dt) {
		const float moveSpeed = 2.0f; // Control velocity directly for more responsive movement
        const float JumpHeight = 0.8f; // The peak height you want to reach, in NDC units.
        // NEW: Calculate initial jump velocity based on desired height and gravity.
        // Formula from your PDF: v = sqrt(2 * g * h)
        const float gravity = std::abs(physics_->gravity_.y);

        const float jumpVelocity = std::sqrt(2.0f * gravity * JumpHeight);

		// Loop through each entity that is in the playerTag map
		for (const auto& [entity, tag] : entityStorage_->playerTag) {
			// Check if player entity has position and velocity components
			//if (entityStorage_->position.find(entity) != entityStorage_->position.end() &&
			//	entityStorage_->velocity.find(entity) != entityStorage_->velocity.end()) {

				// Update the player's position based on its velocity and delta time
				/*entityStorage_->position[entity].Pos.x += entityStorage_->velocity[entity].Vel.x * dt;
				entityStorage_->position[entity].Pos.y += entityStorage_->velocity[entity].Vel.y * dt;*/
				
				// Print player position for debugging
				/*std::cout << "Player Position - X: " << entityStorage_->position[entity].x
						  << ", Y: " << entityStorage_->position[entity].y << std::endl;*/
			

                          // We need a RigidBody, Velocity, and PlayerState to control the player
            if (entityStorage_->velocity.count(entity) && entityStorage_->playerState.count(entity)) {
                auto& vel = entityStorage_->velocity.at(entity);
                auto& state = entityStorage_->playerState.at(entity);

                // --- Horizontal Movement ---
                float targetVelX = 0.0f;
                if (input_->KeyDown(InputSystem::Key::A)) targetVelX = -moveSpeed;
                if (input_->KeyDown(InputSystem::Key::D)) targetVelX = moveSpeed;
                vel.Vel.x = targetVelX;

                // --- Physics-Based Jumping ---
                if (state.isGrounded && (input_->KeyPressed(InputSystem::Key::W) || input_->KeyPressed(InputSystem::Key::Space))) {
                    vel.Vel.y = jumpVelocity; // Apply the calculated upward velocity.
                    
                }
            }
        }
    }


    void PlayerSystem::Shutdown() {
        std::cout << "Player System Shutdown" << std::endl;

        FileIOSystem fileIO;
        PlayerStats newStats;

        for (const auto& [entity, tag] : entityStorage_->playerTag) {
            newStats = entityStorage_->playerStats.at(entity);
            newStats.pos = entityStorage_->position.at(entity);
        }

        fileIO.writePlayerStats(newStats);
    }

} // namespace Framework
