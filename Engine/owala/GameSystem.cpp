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

        //// Player: tag + Position + Velocity + Triangle
        //Triangle ptri{
        //    {-0.12f, -0.08f}, { 0.12f, -0.08f}, { 0.00f, 0.16f},
        //    { 1.0f, 0.35f, 0.25f, 1.0f }
        //};

         // --- Create Player Entity ---
        PlayerStats stats = { 3.0f }; // Example stats
        player_ = createPlayer(*ecs_, stats, { 0.0f, 0.5f }, { 0.05f, 0.08f });

        // Platform: tag + Position + Triangle (no Velocity)
        Triangle tri2{
            {-0.10f, -0.06f}, { 0.10f, -0.06f}, { 0.00f, 0.14f},
            { 0.2f, 0.8f, 1.0f, 1.0f }
        };
        //(void)ecs_->createPlatform("Platform", tri2, /*x,y*/ 0.55f, -0.2f);
        playerTex_ = gfx_->loadTextureFromTex("Assets/duck-rgba-256.tex");
        // Optional texture params
        if (playerTex_) {
            gfx_->setWrap(playerTex_, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            gfx_->setAlphaBlend(true);
        }

        // Create a static ground entity
        ground_ = createEntity();
        ecs_->platformTag[ground_] = PlatformTag{};
        ecs_->position[ground_] = Position{ {0.0f, -0.7f} };
        ecs_->velocity[ground_] = Velocity{}; // Static objects have no velocity
        ecs_->rigidBody[ground_] = RigidBody{};
        ecs_->rigidBody[ground_].SetMass(0.0f); // A mass of 0 makes it static/immovable.
        ecs_->colliders[ground_] = AABBCollider{ {1.0f, 0.2f} }; // A wide, short platform


        //GrateTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png", true);
        //if (GrateTex_) {
        //    std::printf("Successfully loaded Ronald.png\n");
        //    gfx_->setWrap(GrateTex_, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        //    gfx_->setAlphaBlend(true);
        //}

        groundTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png");
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


        // Render the ground
        if (groundTex_ && ecs_->position.count(ground_)) {
            const auto& pos = ecs_->position.at(ground_).Pos;
            const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
            gfx_->drawSprite(groundTex_, pos, halfSize, 0.0f);
        }

        // Render the player
        if (playerTex_ && ecs_->position.count(player_)) {
            const auto& pos = ecs_->position.at(player_).Pos;
            const auto& halfSize = ecs_->colliders.at(player_).HalfSize;
            gfx_->drawSprite(playerTex_, pos, halfSize, 0.0f);
        }

        ////for testing
        ////mutiple ways to use the inputsystem functions using an overload
        //if (input_->KeyPressed(InputSystem::Key::A)) showPlayerSprite_ = !showPlayerSprite_;

        //// NEW: draw the player sprite at its Position (if both exist)
        //if (showPlayerSprite_) {
        //    if (playerTex_) {
        //        auto pit = ecs_->position.find(player_);
        //        if (pit != ecs_->position.end()) {
        //            const auto& p = pit->second;
        //            // center = player position, half size ~0.2 NDC, rotation 0
        //            gfx_->drawSprite(playerTex_, { p.Pos.x, p.Pos.y}, { 0.2f, 0.2f }, 0.0f);
        //        }
        //    }
        //}
        ////for testing 
        //if (input_->KeyPressed('W')) showGrateSprite_ = !showGrateSprite_;

        //// This new helper function will render all entities that have a collider.
        //RenderColliders();

    } // namespace framework

        /* for (const auto& kv : ecs_->triangle) {
        // NEW: draw the player sprite at its Position (if both exist)
        if (showGrateSprite_) {
            if (GrateTex_) {
                auto pit = ecs_->position.find(player_);
                if (pit != ecs_->position.end()) {
                    const auto& p = pit->second;
                    // center = player position, half size ~0.2 NDC, rotation 0
                    gfx_->drawSprite(GrateTex_, { -0.7, -0.7 }, { 0.2f, 0.2f }, 0.0f);
                }
            }
            if (GrateTex_) {
                auto pit = ecs_->position.find(player_);
                if (pit != ecs_->position.end()) {
                    const auto& p = pit->second;
                    // center = player position, half size ~0.2 NDC, rotation 0
                    gfx_->drawSprite(GrateTex_, { 0.7, 0.7 }, { 0.2f, 0.2f }, 0.0f);
                }
            }
            if (GrateTex_) {
                auto pit = ecs_->position.find(player_);
                if (pit != ecs_->position.end()) {
                    const auto& p = pit->second;
                    // center = player position, half size ~0.2 NDC, rotation 0
                    gfx_->drawSprite(GrateTex_, { -0.7, 0.7 }, { 0.2f, 0.2f }, 0.0f);
                }
            }
            if (GrateTex_) {
                auto pit = ecs_->position.find(player_);
                if (pit != ecs_->position.end()) {
                    const auto& p = pit->second;
                    // center = player position, half size ~0.2 NDC, rotation 0
                    gfx_->drawSprite(GrateTex_, { 0.7, -0.7 }, { 0.2f, 0.2f }, 0.0f);
                }
            }
        }
        for (const auto& kv : ecs_->triangle) {
            Entity e = kv.first;
            const Triangle& tri = kv.second;
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
    }*/

    void GameSystem::RenderColliders() {
        // Loop through all entities that have a collider component
        for (auto const& [entity, collider] : ecs_->colliders) {
            // Ensure the entity also has a position
            if (ecs_->position.count(entity)) {
                const auto& pos = ecs_->position.at(entity).Pos;
                const auto& halfSize = collider.HalfSize;

                // Determine the render color based on the entity's tag
                Color col = { 0.8f, 0.8f, 0.8f, 1.0f }; // Default color is grey
                if (ecs_->playerTag.count(entity))   col = { 1.0f, 0.4f, 0.4f, 1.0f }; // Player is red
                if (ecs_->platformTag.count(entity)) col = { 0.4f, 1.0f, 0.4f, 1.0f }; // Platform is green

                // Calculate the min/max corners and issue the draw call
                gfx_->drawRectangle(
                    { pos.x - halfSize.x, pos.y - halfSize.y }, // Min corner (bottom-left)
                    { pos.x + halfSize.x, pos.y + halfSize.y }, // Max corner (top-right)
                    col
                );
            }
        }
    }

} // namespace Framework
