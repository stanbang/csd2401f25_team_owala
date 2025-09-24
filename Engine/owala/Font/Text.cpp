#include "Text.h"

namespace Framework{
	Entity createText(EntityStorage& entityStorage, std::string textToDraw, std::string fontName, Color color, Vector2D pos, float scale)
	{
		Entity txt = createEntity();

		entityStorage.text[txt] = TextComponent{ textToDraw, fontName, color, scale };
		entityStorage.position[txt] = Position{ pos };

		return txt;
	}
}