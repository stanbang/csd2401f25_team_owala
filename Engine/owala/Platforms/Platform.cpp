#include "Platform.h"

namespace Framework {
	Entity createPlatform(EntityStorage& entityStorage, Vector2D pos, Vector2D halfSize, std::string textureID)
	{
		Entity ground = createEntity();

		entityStorage.platformTag[ground] = PlatformTag{};
		entityStorage.position[ground] = Position{ pos };
		entityStorage.velocity[ground] = Velocity{};
		entityStorage.rigidBody[ground] = RigidBody{ {0.2f,0.2} };
		entityStorage.rigidBody[ground].SetMass(0.0f); // A mass of 0 makes it static/immovable.
		entityStorage.colliders[ground] = AABBCollider{ halfSize };
		entityStorage.rotationFlag[ground] = RotationFlag{}; //added rotation for ground
		entityStorage.tex[ground] = TextureComponent{ textureID };

		return ground;
	}
}