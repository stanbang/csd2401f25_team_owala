#include "EcsWorldSystem.h"

namespace Framework {

    Entity EcsWorldSystem::createEntity(const std::string& n) {
        Entity e = nextId_++;
        if (!n.empty()) name[e] = Name{ n };
        return e;
    }

    Entity EcsWorldSystem::createPlayer(const std::string& n,
        const Triangle& tri,
        float x, float y,
        float vx, float vy) {
        Entity e = createEntity(n);
        playerTag[e] = PlayerTag{};         // tag decides component set
        position[e] = { x, y };
        velocity[e] = { vx, vy };          // players move
        triangle[e] = tri;                 // renderable
        return e;
    }

    Entity EcsWorldSystem::createPlatform(const std::string& n,
        const Triangle& tri,
        float x, float y) {
        Entity e = createEntity(n);
        platformTag[e] = PlatformTag{};     // static geometry (no velocity)
        position[e] = { x, y };
        triangle[e] = tri;
        return e;
    }

} // namespace Framework
