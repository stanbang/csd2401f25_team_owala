#include "GameSystem.h"
#include "EcsWorldSystem.h"
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

namespace Framework {

    void GameSystem::Initialize() {
        // One controllable triangle at origin
        player_ = ecs_->createEntity(
            /*pos*/{ 0.0f, 0.0f },
            /*a*/{ -0.12f,-0.08f },
            /*b*/{ 0.12f,-0.08f },
            /*c*/{ 0.00f, 0.16f },
            /*col*/{ 1.0f, 0.35f, 0.25f, 1.0f },
            /*player*/ true
        );

        // One static triangle at the right
        ecs_->createEntity(
            /*pos*/{ 0.55f, -0.2f },
            /*a*/{ -0.10f,-0.06f },
            /*b*/{ 0.10f,-0.06f },
            /*c*/{ 0.00f, 0.14f },
            /*col*/{ 0.2f, 0.8f, 1.0f, 1.0f },
            /*player*/ false
        );
    }

    void GameSystem::Update(float dt) {
        auto* w = win_->window();

        // Input ? movement
        float dx = 0.f, dy = 0.f;
        if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) dy += 1.f;
        if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) dy -= 1.f;
        if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) dx -= 1.f;
        if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) dx += 1.f;

        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.f) { dx /= len; dy /= len; }

        const float speed = 0.9f; // NDC/sec
        dx *= speed * dt; dy *= speed * dt;

        // Move the player
        //did some sort of the collision just for testing
        if (player_ != static_cast<EcsWorldSystem::Entity>(-1)) {
            auto& p = ecs_->get(player_).pos;
            p.x = std::clamp(p.x + dx, -0.95f, 0.95f);
            p.y = std::clamp(p.y + dy, -0.95f, 0.95f);
        }

        // Draw all entities
        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });
        for (std::size_t i = 0; i < ecs_->count(); ++i) {
            const auto& rec = ecs_->get(i);
            const auto& t = rec.tri;
            const auto& p = rec.pos;
            gfx_->drawTriangle(
                { t.a.x + p.x, t.a.y + p.y },
                { t.b.x + p.x, t.b.y + p.y },
                { t.c.x + p.x, t.c.y + p.y },
                t.color
            );
        }
    }
    //void GameSystem::Shutdown(){}     // nothing  for now
} // namespace Framework


//// src/Framework/GameSystem.cpp
//#include "GameSystem.h"
//#include "Graphics.h"
//#include <cmath>
//#include "Player.hpp"
//
//namespace Framework {
//
//	void Initialize() {
//		
//	}   // no-op for now
//
//	void GameSystem::Update(float dt) {
//		t_ += dt;
//
//		// Tell renderer we’re starting the frame and choose a background
//		gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });
//
//		//// Example: draw one animated triangle (frontend code only!)
//		//float wiggle = 0.25f * std::sin(t_ * 2.0f);
//		//Vec2 a{ -0.5f, -0.4f }, b{ 0.5f, -0.4f }, c{ 0.0f,  0.6f + wiggle };
//		//gfx_->drawTriangle(a, b, c, { 1, 0.2f, 0.2f, 1 });
//
//
//		// Background
//		gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });
//		// NDC coords: X,Y in [-1, 1] (left=-1, right=1, bottom=-1, top=1)
//
//		// Left triangle (red)
//		gfx_->drawTriangle({ -0.8f,-0.2f }, { -0.2f,-0.2f }, { -0.5f, 0.4f }, { 1,0,0,1 });
//
//		// Right triangle (green)
//		gfx_->drawTriangle({ 0.2f,-0.2f }, { 0.8f,-0.2f }, { 0.5f, 0.4f }, { 0,1,0,1 });
//
//		// Bottom triangle (blue)
//		gfx_->drawTriangle({ -0.3f,-0.8f }, { 0.3f,-0.8f }, { 0.0f,-0.2f }, { 0,0,1,1 });
//	}
//
//
//	void Shutdown() {}   // no-op for now
//
//} // namespace Framework