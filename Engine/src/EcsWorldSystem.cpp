#include "EcsWorldSystem.h"

namespace Framework {

    EcsWorldSystem::Entity
        EcsWorldSystem::createEntity(const Vec2& pos,
            const Vec2& a, const Vec2& b, const Vec2& c,
            const Color& color,
            bool player)
    {
        Record r;
        r.pos = { pos.x, pos.y };
        r.tri = Triangle{ a, b, c, color };
        r.player = player;

        entities_.push_back(r);
        return entities_.size() - 1; // id is the index
    }

} // namespace Framework
