/*****************************************************************//**
 * \file   PlayerSystem.cpp
 * \brief  
 * 
 * 
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.hpp"
#include "PlayerSystem.hpp"
#include <iostream>

namespace Framework {

	void PlayerSystem::Update(float dt) {
		// Loop through each entity that is in the playerTag map
		for (const auto& [entity, tag] : entityStorage_->playerTag) {
			// Check if player entity has position and velocity components
			if (entityStorage_->position.find(entity) != entityStorage_->position.end() &&
				entityStorage_->velocity.find(entity) != entityStorage_->velocity.end()) {

				// Update the player's position based on its velocity and delta time
				entityStorage_->position[entity].x += entityStorage_->velocity[entity].xVel * dt;
				entityStorage_->position[entity].y += entityStorage_->velocity[entity].yVel * dt;
			}

			// If player entity is missing position or velocity components, handle the error (e.g., log a warning)
			else {
				std::cout << "Player entity detected but without position or velocity component!" << std::endl;
			}
		}
	}
} // namespace Framework
