#include "GameSystem.h"
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Player.h"
#include "EntityStorage.h"
#include "SpriteComponents.h"
#include "InputSystem.h"   // if you want toggles

namespace Framework {

    void GameSystem::Initialize() {
        // Load two PNGs as an example (idle and walk frame)
        Texture idleTex = gfx_->loadTextureFromFile("../Assets/Ronald.png", true);
        Texture walkTex = gfx_->loadTextureFromFile("../Assets/PlayerWalk1.png", true);

        // Build frames (you can push as many frames as you want)
        std::vector<Sprite> frames;
        Sprite idle;
        idle.tex = idleTex;
        idle.halfSize = { 0.20f, 0.20f };   // tweak to fit your art
        idle.rotation = 0.0f;
        idle.tint = { 1,1,1,1 };
        frames.push_back(idle);

        Sprite walk1;
        walk1.tex = walkTex;
        walk1.halfSize = { 0.20f, 0.20f };
        walk1.rotation = 0.0f;
        walk1.tint = { 1,1,1,1 };
        frames.push_back(walk1);

        // Create the player with sprite frames (at 0,0)
        player_ = createPlayer(*ecs_, frames, 0.0f, 0.0f);

        // (optional) enable alpha blending for PNG transparency
        gfx_->setAlphaBlend(true);
        // (optional) clamp wrap
        if (idleTex) gfx_->setWrap(idleTex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        if (walkTex) gfx_->setWrap(walkTex, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
    }

    void GameSystem::Update(float dt) {
        // Clear

            // fetch components safely
        auto pIt = ecs_->position.find(player_);
        auto vIt = ecs_->velocity.find(player_);
        if (pIt == ecs_->position.end() || vIt == ecs_->velocity.end())
            return;

        Position& pos = pIt->second;
        Velocity& vel = vIt->second;

        // input -> velocity
        float vx = 0.f, vy = 0.f;
        if (input_ && input_->KeyDown(InputSystem::Key::W)) vy += 1.f;
        if (input_ && input_->KeyDown(InputSystem::Key::S)) vy -= 1.f;
        if (input_ && input_->KeyDown(InputSystem::Key::A)) vx -= 1.f;
        if (input_ && input_->KeyDown(InputSystem::Key::D)) vx += 1.f;

        float len = std::sqrt(vx * vx + vy * vy);
        if (len > 0.f) { vx /= len; vy /= len; }

        const float speed = 0.9f;
        vel.xVel = vx * speed;
        vel.yVel = vy * speed;

        // integrate here
        pos.x += vel.xVel * dt;
        pos.y += vel.yVel * dt;

        // optional: clamp to NDC [-1, 1]
        pos.x = std::clamp(pos.x, -1.0f, 1.0f);
        pos.y = std::clamp(pos.y, -1.0f, 1.0f);

        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

        // Draw current player sprite if we have both Position and Sprites
        std::unordered_map<Entity, Position>::const_iterator pit = ecs_->position.find(player_);
        std::unordered_map<Entity, Sprites>::const_iterator  sit = ecs_->sprites.find(player_);

        if (pit != ecs_->position.end() && sit != ecs_->sprites.end()) {
            const Position& pos = pit->second;
            const Sprites& sp = sit->second;

            if (!sp.frames.empty()) {
                const int idx = (sp.current >= 0 && sp.current < (int)sp.frames.size()) ? sp.current : 0;
                const Sprite& cur = sp.frames[(size_t)idx];
                gfx_->drawSprite(cur.tex, { pos.x, pos.y }, cur.halfSize, cur.rotation);
            }
        }
    }

} // namespace Framework
