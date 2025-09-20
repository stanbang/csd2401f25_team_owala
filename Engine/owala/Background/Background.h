#include "Entity.h"
#include "EntityStorage.h"
#include <string>

namespace Framework{
	Entity createBackground(EntityStorage& entityStorage, Vector2D pos, std::string textureID);
}	