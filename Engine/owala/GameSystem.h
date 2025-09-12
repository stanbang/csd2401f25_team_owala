#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "InputSystem.h"

namespace Framework {

    class Graphics;
    class WindowsSystem;

    class GameSystem : public ISystem {
    public:
        GameSystem(EntityStorage& ecs, Graphics& gfx, WindowsSystem& win, InputSystem& input)
            : ecs_(&ecs), gfx_(&gfx), win_(&win), input_(&input) {
        }

        void Initialize() override;     // spawn player/platform via tag-aware creators
        void Update(float dt) override; // WASD for player + draw all triangles
        void Shutdown() override {}     // nothing owned

    private:
        void RenderColliders(); // Helper to draw all colliders

        EntityStorage* ecs_ = nullptr;
        Graphics* gfx_ = nullptr;
        WindowsSystem* win_ = nullptr;
        InputSystem* input_ = nullptr;   // <-- MISSING MEMBER (add this)

        Entity          player_{ 0 };
        Entity ground_{ 0 };

        Texture playerTex_ = 0;   //
        Texture GrateTex_ = 0;
        bool showPlayerSprite_ = false;  // just for testing 
        bool showGrateSprite_ = false;  // just for testing 


    };

} // namespace Framework
