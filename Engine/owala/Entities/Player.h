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
#include "Animation.h"
#include "ResourceManager.h"
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
		ResourceManager& rcm,
		Vector2D halfSize, 
		std::string textureKey) {

			//Create an EntityManager to access entity creation APIs
			EntityManager entityMgr(entityStorage);
			(void) rcm; // suppress warning level
			//Create a new entity with a unique ID
			Entity player = entityMgr.CreateEntity();
			/*Assign components to the player entity, and
			set initial values in the unordered maps.*/
			//entityStorage.triangle[player] = triangle;
			entityMgr.AddComponent(player, PlayerTag{});
			entityMgr.AddComponent(player, Position{ Vector2D{} }); //Start at 0,0 coordinates
			entityMgr.AddComponent(player, Velocity{});  //Start with no velocity

			entityMgr.AddComponent(player, TextureComponent{ textureKey });
			entityMgr.AddComponent(player, AnimationComponent{AnimationState::IDLE});
			entityMgr.AddComponent(player, RigidBody{});
			entityMgr.GetComponent<RigidBody>(player).SetMass(1.0f); // Player has mass

			entityMgr.AddComponent(player, AABBCollider{ halfSize });
			//Assign the player to the Player collision layer.
			EntityProperties& playerProperties = entityMgr.GetProperties(player);

			playerProperties.name = "Player";
			playerProperties.layer = CollisionLayer::Player;
			entityMgr.AddComponent(player, PlayerState{}); //  Add the player state component
			entityMgr.AddComponent(player, RotationFlag{}); // added rotation flag

			//Return the entity ID of the player
			return player;
	}

	

	inline void updatePlayerSize(EntityStorage& entityStorage, Entity entity, float scale) {
		//Create an EntityManager to access entity creation APIs
		EntityManager entityMgr(entityStorage);
		if (!entityMgr.HasComponent(entity, AABBColliderType)) {
			Logger::Get().Error(DebugType::PlayerSystem) << "Unable to update player size. Player is missing AABBCollider!" << std::endl;
			return;
		}
		AABBCollider& aabb = entityMgr.GetComponent<AABBCollider>(entity);
		aabb.HalfSize.x *= scale;
		aabb.HalfSize.y *= scale;
	}
}