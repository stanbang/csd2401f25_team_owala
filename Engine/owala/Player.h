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

	//Entity createPlayer(EntityStorage& entityStorage, const Triangle& triangle, float startXPos = 0.0f, float startYPos = 0.0f) {
	inline Entity createPlayer(EntityStorage& entityStorage, 
		PlayerStats stats, 
		Vector2D startPos, 
		Vector2D halfSize, 
		std::shared_ptr<Framework::Texture> texture) {

			//Create a new entity with a unique ID
			Entity player = createEntity();
			/*Assign components to the player entity, and
			set initial values in the unordered maps.*/
			//entityStorage.triangle[player] = triangle;
			entityStorage.playerTag[player] = PlayerTag{}; //Assign PlayerTag component to the player entity
			entityStorage.playerStats[player] = stats;
			entityStorage.position[player] = Position{ startPos }; //Start at 0,0 coordinates
			entityStorage.velocity[player] = Velocity{}; //Start with no velocity

			entityStorage.tex[player] = TextureComponent{ texture };
			entityStorage.rigidBody[player] = RigidBody{};
			entityStorage.rigidBody[player].SetMass(1.0f); // Player has mass

			entityStorage.colliders[player] = AABBCollider{ halfSize };
			entityStorage.playerState[player] = PlayerState{}; // NEW: Add the player state component
			//Return the entity ID of the player
			return player;
	}

	//// NEW: create a player with a vector of sprites (frames)
	//inline Entity createPlayer(EntityStorage& es,
	//	const std::vector<Sprite>& frames,
	//	Vector2D startPos)
	//{
	//	Entity player = createEntity();
	//	es.playerTag[player] = PlayerTag{};
	//	es.position[player] = Position{ startPos };
	//	es.velocity[player] = Velocity{ };

	//	Sprites s;
	//	s.frames = frames;      // copy your frames
	//	s.current = 0;          // start on first frame
	//	es.sprites[player] = s; // attach Sprites component

	//	return player;
	//}
}