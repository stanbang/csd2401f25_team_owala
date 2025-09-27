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
#include "EntityManager.h"
#include <string>

/**
 * \function createPlayer
 * 
 * \param startXPos The start position of the player on the x-axis
 * \param startYPos The start position of the player on the y-axis
 * \return The entity ID of the created player
 */
namespace Framework {

	inline Entity createPlayer(EntityStorage& entityStorage, 
		PlayerStats stats, 
		Vector2D halfSize, 
		std::string textureID) {

			//Create an EntityManager to access entity creation APIs
			EntityManager entityMgr(entityStorage);

			//Create a new entity with a unique ID
			Entity player = entityMgr.CreateEntity();
			/*Assign components to the player entity, and
			set initial values in the unordered maps.*/
			//entityStorage.triangle[player] = triangle;
			entityMgr.AddComponent(player, PlayerTag{});
			entityMgr.AddComponent(player, stats);
			entityMgr.AddComponent(player, Position{ stats.pos }); //Start at 0,0 coordinates
			entityMgr.AddComponent(player, Velocity{});  //Start with no velocity

			//entityStorage.tex[player] = TextureComponent{ texture };
			entityMgr.AddComponent(player, TextureComponent{ textureID });
			entityMgr.AddComponent(player, RigidBody{});
			entityMgr.GetComponent<RigidBody>(player).SetMass(1.0f); // Player has mass

			entityMgr.AddComponent(player, AABBCollider{ halfSize });
			entityMgr.AddComponent(player, PlayerState{}); // NEW: Add the player state component
			entityMgr.AddComponent(player, RotationFlag{}); // added rotation flag
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