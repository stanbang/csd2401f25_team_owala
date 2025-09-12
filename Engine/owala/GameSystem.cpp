#include "GameSystem.h"
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>      // <-- for GL_CLAMP_TO_EDGE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include "Player.h"
#include "EntityStorage.h"

namespace Framework {

    void GameSystem::Initialize() {

        // Player: tag + Position + Velocity + Triangle
        Triangle ptri{
            {-0.12f, -0.08f}, { 0.12f, -0.08f}, { 0.00f, 0.16f},
            { 1.0f, 0.35f, 0.25f, 1.0f }
        };

        // Create the player entity using the new, corrected createPlayer function.
         // It starts at position (0, 0.5) with a physical size of 0.1x0.1 NDC units.
        player_ = createPlayer(*ecs_, { 0.0f, 0.5f }, { 0.05f, 0.05f });

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
        ecs_->position[ground_] = Position{ {0.0f, -0.8f} };
        ecs_->velocity[ground_] = Velocity{};
        ecs_->rigidBody[ground_] = RigidBody{};
        ecs_->rigidBody[ground_].SetMass(0.0f); // A mass of 0 makes it static/immovable.
        ecs_->colliders[ground_] = AABBCollider{ {1.0f, 0.1f} };
        GrateTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png", true);
        if (GrateTex_) {
            std::printf("Successfully loaded Ronald.png\n");
            gfx_->setWrap(GrateTex_, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            gfx_->setAlphaBlend(true);
        }
    }

    void GameSystem::Update(float dt) {
        GLFWwindow* w = win_->window();

        // --- Render: all entities with (Position + Triangle) ---
        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

        //for testing
        //mutiple ways to use the inputsystem functions using an overload
        if (input_->KeyPressed(InputSystem::Key::A)) showPlayerSprite_ = !showPlayerSprite_;
        
        // NEW: draw the player sprite at its Position (if both exist)
        if (showPlayerSprite_){
            if (playerTex_) {
                auto pit = ecs_->position.find(player_);
                if (pit != ecs_->position.end()) {
                    const auto& p = pit->second;
                    // center = player position, half size ~0.2 NDC, rotation 0
                    gfx_->drawSprite(playerTex_, { p.x, p.y }, { 0.2f, 0.2f }, 0.0f);
                }
            }
        }
        //for testing 
        if (input_->KeyPressed('W')) showGrateSprite_ = !showGrateSprite_;

        // This new helper function will render all entities that have a collider.
        RenderColliders();

        /*for (const auto& kv : ecs_->triangle) {
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

            auto itP = ecs_->position.find(e);
            if (itP == ecs_->position.end()) continue;
            const Position& p = itP->second;

            gfx_->drawTriangle(
                { tri.a.x + p.x, tri.a.y + p.y },
                { tri.b.x + p.x, tri.b.y + p.y },
                { tri.c.x + p.x, tri.c.y + p.y },
                tri.color
            );
        }
    }

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
