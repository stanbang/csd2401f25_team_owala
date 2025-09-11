// src/Framework/GameSystem.cpp
#include "GameSystem.h"
#include "Graphics.h"
#include "PhysicsSystem.h"
#include "Core.h"
#include "Entity.hpp"
#include <cmath>


namespace Framework {

	void GameSystem::Initialize() {

		// Create our first entity!
		fallingBox = createEntity();
		// Add components to the entity using the physics system
		physics_->AddTransform(fallingBox, { 0.0f, 0.5f }); // Start it half-way up the screen
		physics_->AddRigidBody(fallingBox, false); // `false` for a dynamic object

		// Create a static ground plane
		groundPlane = createEntity();
		physics_->AddTransform(groundPlane, { 0.0f, -0.8f });
		physics_->AddRigidBody(groundPlane, true); // `true` makes it static
	}   // no-op for now

	void GameSystem::Update(float dt) {
		t_ += dt;

		////// Tell renderer we’re starting the frame and choose a background
		////gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

		////// Example: draw one animated triangle (frontend code only!)
		////float wiggle = 0.25f * std::sin(t_ * 2.0f);
		////Vec2 a{ -0.5f, -0.4f }, b{ 0.5f, -0.4f }, c{ 0.0f,  0.6f + wiggle };
		////gfx_->drawTriangle(a, b, c, { 1, 0.2f, 0.2f, 1 });


		//// Background
		//gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

		//// NDC coords: X,Y in [-1, 1] (left=-1, right=1, bottom=-1, top=1)

		//// Left triangle (red)
		//gfx_->drawTriangle({ -0.8f,-0.2f }, { -0.2f,-0.2f }, { -0.5f, 0.4f }, { 1,0,0,1 });

		//// Right triangle (green)
		//gfx_->drawTriangle({ 0.2f,-0.2f }, { 0.8f,-0.2f }, { 0.5f, 0.4f }, { 0,1,0,1 });

		//// Bottom triangle (blue)
		//gfx_->drawTriangle({ -0.3f,-0.8f }, { 0.3f,-0.8f }, { 0.0f,-0.2f }, { 0,0,1,1 });

		 // --- Rendering ---
		gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

		// Render the falling box
		{
			TransformComponent& transform = physics_->GetTransform(fallingBox);
			Vector2D pos = transform.position;
			float size = 0.1f;
			Vec2 p1 = { pos.x - size, pos.y - size }; // Bottom-left
			Vec2 p2 = { pos.x + size, pos.y - size }; // Bottom-right
			Vec2 p3 = { pos.x - size, pos.y + size }; // Top-left
			Vec2 p4 = { pos.x + size, pos.y + size }; // Top-right
			gfx_->drawTriangle(p1, p2, p3, { 1.0f, 0.2f, 0.2f, 1.0f });
			gfx_->drawTriangle(p3, p2, p4, { 1.0f, 0.2f, 0.2f, 1.0f });
		}

		// Render the ground plane
		{
			TransformComponent& transform = physics_->GetTransform(groundPlane);
			Vector2D pos = transform.position;
			float width = 1.0f;
			float height = 0.05f;
			Vec2 p1 = { pos.x - width, pos.y - height }; // Bottom-left
			Vec2 p2 = { pos.x + width, pos.y - height }; // Bottom-right
			Vec2 p3 = { pos.x - width, pos.y + height }; // Top-left
			Vec2 p4 = { pos.x + width, pos.y + height }; // Top-right
			gfx_->drawTriangle(p1, p2, p3, { 0.2f, 0.8f, 0.2f, 1.0f });
			gfx_->drawTriangle(p3, p2, p4, { 0.2f, 0.8f, 0.2f, 1.0f });
		}

		gfx_->endFrame();
	}


	void GameSystem::Shutdown() {}   // no-op for now

} // namespace Framework
