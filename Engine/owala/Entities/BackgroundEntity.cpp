#include "BackgroundEntity.h"
#include "EntityManager.h"

namespace Framework{
	Entity createBackground(EntityStorage& entityStorage, Vector2D pos, std::string textureID)
	{
		EntityManager entityMgr(entityStorage);
		Entity bg = entityMgr.CreateEntity();

		entityMgr.AddComponent(bg, BackgroundTag{});
		entityMgr.AddComponent(bg, Position{ pos });
		entityMgr.AddComponent(bg, TextureComponent{ textureID ,  {1.0f, 1.0f } });

		return bg;
	}
}