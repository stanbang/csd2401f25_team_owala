/*****************************************************************//**
* \file   Fake_Player.h
*
* \primary_author
*      Ng Jing Wen (50%)
* \secondary_author
*	   Stanley Lur (50%)
*
*
* \brief	Contains declaration and definitions to create a Fake player entity.
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
		std::uniform_real_distribution<float> PosX_Distrib(-1.0f, 1.0f);
		std::uniform_real_distribution<float> PosY_Distrib(-1.0f, 1.0f);

		//Generate random scale/halfsize between 0.01 to 0.005
		std::uniform_real_distribution<float> Scale_DistribX(0.005f, 0.1f);
		std::uniform_real_distribution<float> Scale_DistribY(0.005f, 0.1f);

		Vector2D randomPos = { PosY_Distrib(gen), PosX_Distrib(gen) };
		Vector2D randomScale = { Scale_DistribX(gen), Scale_DistribX(gen) };

		//Create an EntityManager to access entity creation APIs
		EntityManager entityMgr(entityStorage);
		(void)rcm; // suppress warning level
		(void)halfSize; // suppress warning level
		//Create a new entity with a unique ID
		Entity fake_player = entityMgr.CreateEntity();
		
		entityMgr.AddComponent(fake_player, PlayerTag{});
		
		entityMgr.AddComponent(fake_player, Position{ randomPos });  // Start with rand Pos
		

		entityMgr.AddComponent(fake_player, TextureComponent{ textureKey , randomScale });
		
		//Return the entity ID of the fake player
		return fake_player;
	}

}