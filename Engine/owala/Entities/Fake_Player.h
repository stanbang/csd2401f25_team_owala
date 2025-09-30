/*****************************************************************//**
* \file   Fake_Player.h
*
* \primary_author
*      Ng Jing Wen
* \secondary_author
* Stanley
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
#include <random>

// FOR TESTING FOR 2500 PLAYER SPRITES

namespace Framework {
	inline Entity createFakePlayer(EntityStorage& entityStorage,
		ResourceManager& rcm,
		Vector2D halfSize,
		std::string textureKey) {

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> distribX(-1.0f, 1.0f);
		std::uniform_real_distribution<float> distribY(-1.0f, 1.0f);

		Vector2D randomPos = { distribX(gen), distribY(gen) };

		//Create an EntityManager to access entity creation APIs
		EntityManager entityMgr(entityStorage);

		//Create a new entity with a unique ID
		Entity player = entityMgr.CreateEntity();
		/*Assign components to the player entity, and
		set initial values in the unordered maps.*/
		//entityStorage.triangle[player] = triangle;
		entityMgr.AddComponent(player, PlayerTag{});
		
		entityMgr.AddComponent(player, Position{randomPos}); //Start at 0,0 coordinates
		

		entityMgr.AddComponent(player, TextureComponent{ textureKey });
		
		//Return the entity ID of the player
		return player;
	}

}