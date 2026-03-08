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
		Vector2D position,  // Position from grid
		std::string textureKey) {

		std::random_device rd;
		std::mt19937 gen(rd());

		// Generate random size between 0.02 to 0.08 (visible but varied)
		std::uniform_real_distribution<float> sizeDistrib(0.02f, 0.08f);

		// Generate random velocity between -0.5 to 0.5 for both x and y
		std::uniform_real_distribution<float> velDistrib(-0.5f, 0.5f);

		// Create random size, scale, and velocity
		float randomSize = sizeDistrib(gen);
		Vector2D randomScale = { randomSize, randomSize };
		Vector2D randomVelocity = { velDistrib(gen), velDistrib(gen) };

		// Create an EntityManager to access entity creation APIs
		EntityManager entityMgr(entityStorage);
		(void)rcm; // suppress warning level

		// Create a new entity with a unique ID
		Entity fake_player = entityMgr.CreateEntity();

		entityMgr.AddComponent(fake_player, FakePlayerTag{});
		entityMgr.AddComponent(fake_player, Transform{ position });  // Use grid position
		entityMgr.AddComponent(fake_player, TextureComponent{ textureKey, randomScale });
		entityMgr.AddComponent(fake_player, AnimationComponent{ "FakePlayer", "IDLE" });
		entityMgr.AddComponent(fake_player, LoadedTag{});  // Critical for rendering

		// Return the entity ID of the fake player
		return fake_player;
	}

}