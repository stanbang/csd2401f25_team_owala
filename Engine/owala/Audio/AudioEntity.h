#pragma once
#include <string>
#include "Entity.h"
#include "Components.h"
#include "EntityStorage.h"

namespace Framework{
	Entity createBGAudio(EntityStorage& entityStorage,
		PlayerStats stats,
		Vector2D halfSize,
		std::string audioID) {

		//Create a new entity with a unique ID
		Entity bg_audio = createEntity();
		/*Assign components to the player entity, and
		set initial values in the unordered maps.*/

		//entityStorage.tex[player] = TextureComponent{ texture };
		entityStorage.audio[bg_audio] = AudioComponent{ audioID, true, true };

		//Return the entity ID of the player
		return bg_audio;
	}
}