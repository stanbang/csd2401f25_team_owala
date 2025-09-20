#include "Entity.h"
#include "Components.h"
#include "EntityStorage.h"
#include "FileUtility.h"
#include <string>

namespace Framework{
	Entity createPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID);
}