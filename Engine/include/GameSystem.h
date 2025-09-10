#pragma once
#include "ISystem.h"
#include "EcsWorldSystem.h"  // for Entity + simple ECS

namespace Framework {

    class Graphics;
    class WindowsSystem;

    // Gameplay system: spawns one controllable triangle, moves it with WASD,
    // and draws all entities each frame.
    class GameSystem : public ISystem {
    public:
        GameSystem(EcsWorldSystem& ecs, Graphics& gfx, WindowsSystem& win)
            : ecs_(&ecs), gfx_(&gfx), win_(&win) {
        }

        void Initialize() override;     // calls ecs_->createEntity(...)
        void Update(float dt) override; // input + move + draw
        void Shutdown() override {}     // nothing owned

    private:
        EcsWorldSystem* ecs_ = nullptr;
        Graphics* gfx_ = nullptr;
        WindowsSystem* win_ = nullptr;

        // id from ecs_->createEntity(...) for the controllable triangle
        EcsWorldSystem::Entity player_{ static_cast<EcsWorldSystem::Entity>(-1) };
    };

} // namespace Framework
