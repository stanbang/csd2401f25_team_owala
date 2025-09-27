/*****************************************************************//**
* \file   Player.h
*
* \primary_author
*      Ng Jing Wen
* \secondary_author
*
*
* \brief	Contains declaration and definitions to create a player entity.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
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

	void updatePlayerSize(EntityStorage& entityStorage, Entity entity, float diff) {
		//Create an EntityManager to access entity creation APIs
		EntityManager entityMgr(entityStorage);
		entityMgr.GetComponent<AABBCollider>(entity).HalfSize.x += diff;
		entityMgr.GetComponent<AABBCollider>(entity).HalfSize.y += diff;
	}
}