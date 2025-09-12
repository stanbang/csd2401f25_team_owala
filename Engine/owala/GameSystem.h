#pragma once
#include "ISystem.h"
#include "EntityStorage.h"

namespace Framework {

    class Graphics;
    class WindowsSystem;

    class GameSystem : public ISystem {
    public:
        GameSystem(EntityStorage& ecs, Graphics& gfx, WindowsSystem& win)
            : ecs_(&ecs), gfx_(&gfx), win_(&win) {
        }

        void Initialize() override;     // spawn player/platform via tag-aware creators
        void Update(float dt) override; // WASD for player + draw all triangles
        void Shutdown() override {}     // nothing owned

    private:
        void RenderColliders(); // Helper to draw all colliders

        EntityStorage* ecs_ = nullptr;
        Graphics* gfx_ = nullptr;
        WindowsSystem* win_ = nullptr;

        Entity          player_{ 0 };
        Entity ground_{ 0 };

        Texture playerTex_ = 0;   //
    };

} // namespace Framework
