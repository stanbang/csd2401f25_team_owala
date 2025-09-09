/*****************************************************************//**
 * \file	Player.hpp
 * \brief	Contains function to create a player entity with
 *			its respective components
 * 
 * \author	Ng Jing Wen
 * \date	10 September 2025
 *********************************************************************/
#pragma once
#include "Entity.hpp"
#include "Components.hpp"
#include "EntityStorage.hpp"

/**
 * \function createPlayer
 * 
 * \param startXPos The start position of the player on the x-axis
 * \param startYPos The start position of the player on the y-axis
 * \return The entity ID of the created player
 */
Entity createPlayer(float startXPos = 0.0f, float startYPos = 0.0f){
	//Create a new entity with a unique ID
	Entity player = createEntity();

	/*Assign components to the player entity, and 
	set initial values in the unordered maps.*/
	playerTag[player] = PlayerTag{}; //Assign PlayerTag component to the player entity
	position[player] = Position{ startXPos, startYPos }; //Start at 0,0 coordinates
	velocity[player] = Velocity{ 0.0f, 0.0f }; //Start with no velocity

	//Return the entity ID of the player
	return player;
}
