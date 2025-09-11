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
        player_ = createPlayer(*ecs_, ptri, /*x,y*/ 0.f, 0.f);

        // Platform: tag + Position + Triangle (no Velocity)
        Triangle tri2{
            {-0.10f, -0.06f}, { 0.10f, -0.06f}, { 0.00f, 0.14f},
            { 0.2f, 0.8f, 1.0f, 1.0f }
        };
        //(void)ecs_->createPlatform("Platform", tri2, /*x,y*/ 0.55f, -0.2f);
        playerTex_ = gfx_->loadTextureFromTex("../Assets/duck-rgba-256.tex");
        // Optional texture params
        if (playerTex_) {
            gfx_->setWrap(playerTex_, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            gfx_->setAlphaBlend(true);
        }
        GrateTex_ = gfx_->loadTextureFromFile("../Assets/Ronald.png", true);
        if (GrateTex_) {
            std::printf("Successfully loaded Ronald.png\n");
            gfx_->setWrap(GrateTex_, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
            gfx_->setAlphaBlend(true);
        }
    }

    void GameSystem::Update(float dt) {
        GLFWwindow* w = win_->window();

        // --- WASD ? normalized direction ---
        //float dx = 0.f, dy = 0.f;
        //if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) dy += 1.f;
        //if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) dy -= 1.f;
        //if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.f;
        //if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) dx += 1.f;

        //float len = std::sqrt(dx * dx + dy * dy);
        //if (len > 0.f) { dx /= len; dy /= len; }

        //const float speed = 0.9f;  // NDC units/sec
        //dx *= speed * dt; dy *= speed * dt;

        // --- Move the player with clamping based on its triangle extents ---
        /*auto pit = ecs_->position.find(player_);
        if (pit != ecs_->position.end()) {
            auto& p = pit->second;

            float hx = 0.05f, hy = 0.05f;
            auto tit = ecs_->triangle.find(player_);
            if (tit != ecs_->triangle.end()) {
                const auto& t = tit->second;
                hx = std::max({ std::fabs(t.a.x), std::fabs(t.b.x), std::fabs(t.c.x) });
                hy = std::max({ std::fabs(t.a.y), std::fabs(t.b.y), std::fabs(t.c.y) });
            }

            p.x = std::clamp(p.x + dx, -1.0f + hx, 1.0f - hx);
            p.y = std::clamp(p.y + dy, -1.0f + hy, 1.0f - hy);
        }*/

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
        }
        /*for (const auto& kv : ecs_->triangle) {
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
        }*/
    }

} // namespace Framework
