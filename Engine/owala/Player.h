/*****************************************************************//**
 * \file	Player.hpp
 * \brief	Contains function to create a player entity with
 *			its respective components
 * 
 * \author	Ng Jing Wen
 * \date	10 September 2025
 *********************************************************************/
#pragma once
#include "Entity.h"
#include "Components.h"
#include "EntityStorage.h"
#include "FileUtility.h"

/**
 * \function createPlayer
 * 
 * \param startXPos The start position of the player on the x-axis
 * \param startYPos The start position of the player on the y-axis
 * \return The entity ID of the created player
 */
namespace Framework {
	FileIOSystem fileIO;
	PlayerStats playerStats = fileIO.loadPlayerStats();

	Entity createPlayer(EntityStorage& entityStorage, const Triangle& triangle, float startXPos = 0.0f, float startYPos = 0.0f) {
		//Create a new entity with a unique ID
		Entity player = createEntity();
		/*Assign components to the player entity, and
		set initial values in the unordered maps.*/
		//entityStorage.triangle[player] = triangle;
		entityStorage.playerTag[player] = PlayerTag{}; //Assign PlayerTag component to the player entity
		entityStorage.playerStats[player] = playerStats;
		entityStorage.position[player] = Position{ startXPos, startYPos }; //Start at 0,0 coordinates
		entityStorage.velocity[player] = Velocity{ 0.0f, 0.0f }; //Start with no velocity

		entityStorage.velocity[player] = Velocity{}; //Start with no velocity

		entityStorage.rigidBody[player] = RigidBody{};
		entityStorage.rigidBody[player].SetMass(1.0f); // Player has mass

		entityStorage.colliders[player] = AABBCollider{ halfSize };
		//Return the entity ID of the player
		return player;
	}
}