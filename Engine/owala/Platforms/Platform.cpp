#include "Platform.h"
#include "EntityManager.h"

namespace Framework {
	Entity createPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID)
	{
		EntityManager entityMgr(entityStorage);
		Entity ground = entityMgr.CreateEntity();

		entityMgr.AddComponent(ground, PlatformTag{});
		entityMgr.AddComponent(ground, Position{ pos });
		entityMgr.AddComponent(ground, Velocity{});
		entityMgr.AddComponent(ground, RigidBody{ {0.2f,0.2} });
		entityMgr.GetComponent<RigidBody>(ground).SetMass(0.0f); // A mass of 0 makes it static/immovable.
		entityMgr.AddComponent(ground, AABBCollider{ {1.0f, 0.2f} });
		entityMgr.AddComponent(ground, RotationFlag{});
		entityMgr.AddComponent(ground, TextureComponent{ textureID });

		return ground;
	}
}