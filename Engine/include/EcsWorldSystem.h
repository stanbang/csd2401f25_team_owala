#pragma once
#include "ISystem.h"
#include "Graphics.h"   // for Vec2, Color
#include <vector>
#include <cstddef>

namespace Framework {

    struct Position { float x, y; };
    struct Triangle { Vec2 a, b, c; Color color; };

    class EcsWorldSystem : public ISystem {
    public:
        using Entity = std::size_t;

        struct Record {
            Position pos;
            Triangle tri;
            bool     player = false;
        };

        // ISystem lifecycle
        void Initialize() override {}
        void Update(float) override {}
        void Shutdown() override { entities_.clear(); }

        //Create one entity (triangle) with provided values; returns its id.
        Entity createEntity(const Vec2& pos,
            const Vec2& a, const Vec2& b, const Vec2& c,
            const Color& color,
            bool player = false);

        //memeber functions Access
        Record& get(Entity id) { return entities_[id]; }
        const Record& get(Entity id) const { return entities_[id]; }
        std::size_t   count() const { return entities_.size(); }

    private:
        std::vector<Record> entities_;
    };

} // namespace Framework
