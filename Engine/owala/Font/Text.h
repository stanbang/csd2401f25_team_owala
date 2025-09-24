#include "Entity.h"
#include "EntityStorage.h"
#include <string>

namespace Framework{
	Entity createText(EntityStorage& entityStorage, std::string textToDraw, std::string fontName, Color color, Vector2D pos, float scale);
}	