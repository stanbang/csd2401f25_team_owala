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

namespace Framework {

	void PlayerSystem::Update(float dt) {
		const float moveForce = 5.0f; // Force to apply for movement
		const float jumpForce = 10.0f; // An impulse-like force for jumping


		// Loop through each entity that is in the playerTag map
		for (const auto& [entity, tag] : entityStorage_->playerTag) {
			// Check if player entity has position and velocity components
			//if (entityStorage_->position.find(entity) != entityStorage_->position.end() &&
			//	entityStorage_->velocity.find(entity) != entityStorage_->velocity.end()) {

			//	// Update the player's position based on its velocity and delta time
			//	entityStorage_->position[entity].Pos.x += entityStorage_->velocity[entity].Vel.x * dt;
			//	entityStorage_->position[entity].Pos.y += entityStorage_->velocity[entity].Vel.y * dt;
			//	
			//	// Print player position for debugging
			//	std::cout << "Player Position - X: " << entityStorage_->position[entity].Pos.x
			//		<< ", Y: " << entityStorage_->position[entity].Pos.y << std::endl;
			//}

			//// If player entity is missing position or velocity components, handle the error (e.g., log a warning)
			//else {
			//	std::cout << "Player entity detected but without position or velocity component!" << std::endl;
			//}
			// Check if the player entity has a RigidBody component to apply forces to
			if (entityStorage_->rigidBody.count(entity)) {
				auto& body = entityStorage_->rigidBody.at(entity);

				// Left/Right Movement
				if (glfwGetKey(win_->window(), GLFW_KEY_A) == GLFW_PRESS) {
					body.Force.x -= moveForce;
				}
				if (glfwGetKey(win_->window(), GLFW_KEY_D) == GLFW_PRESS) {
					body.Force.x += moveForce;
				}

				// Jumping (apply as an upward force)
				if (glfwGetKey(win_->window(), GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win_->window(), GLFW_KEY_SPACE) == GLFW_PRESS) {
					// This is a simple jump. For better control, you'd check if the player is on the ground first.
					body.Force.y += jumpForce;
				}
			}
		}
	}
} // namespace Framework
