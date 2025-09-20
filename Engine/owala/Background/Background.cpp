#include "Background.h"

namespace Framework{
	Entity createBackground(EntityStorage& entityStorage, Vector2D pos, std::string textureID)
	{
		Entity bg = createEntity();

		entityStorage.backgroundTag[bg] = BackgroundTag{};
		entityStorage.position[bg] = Position{ pos };
		entityStorage.tex[bg] = TextureComponent{ textureID };

		return bg;
	}
}