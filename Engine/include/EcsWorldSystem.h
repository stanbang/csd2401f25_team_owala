#pragma once
#include "ISystem.h"
#include "Graphics.h"            // for Framework::Vec2, Framework::Color
#include <unordered_map>
#include <string>
#include <cstdint>

namespace Framework {

    using Entity = std::uint32_t;

    // ---------- Tags ----------
    struct PlayerTag {};
    struct PlatformTag {};
    struct EnemyTag {};

    // ---------- Components ----------
    struct Name { std::string value; };
    struct Position { float x, y; };
    struct Velocity { float x, y; };
    struct Acceleration { float x, y; };
    struct Triangle { Vec2 a, b, c; Color color; };

    class EcsWorldSystem : public ISystem {
    public:
        void Initialize() override {}
        void Update(float) override {}
        void Shutdown() override {}

        // Core entity API
        Entity createEntity(const std::string& name = "");

        // Tag-aware creators (enforce which components a tag must have)
        Entity createPlayer(const std::string& name,
            const Triangle& tri,
            float x, float y,
            float vx = 0.f, float vy = 0.f);

        Entity createPlatform(const std::string& name,
            const Triangle& tri,
            float x, float y);

        // Storages (simple explicit maps)
        std::unordered_map<Entity, Name>         name;
        std::unordered_map<Entity, PlayerTag>    playerTag;
        std::unordered_map<Entity, PlatformTag>  platformTag;
        std::unordered_map<Entity, EnemyTag>     enemyTag;

        std::unordered_map<Entity, Position>     position;
        std::unordered_map<Entity, Velocity>     velocity;
        std::unordered_map<Entity, Acceleration> acceleration;
        std::unordered_map<Entity, Triangle>     triangle;

    private:
        Entity nextId_ = 1;
    };

} // namespace Framework
