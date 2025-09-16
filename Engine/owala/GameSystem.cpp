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
#include "RotationSystem.h"
#include "Matrix3x3.h"

#include "SpriteComponents.h"
#include "InputSystem.h"   // if you want toggles
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "PhysicsSystem.h"



namespace Framework {

    void GameSystem::Initialize() {
        // --- Load Assets ---
        // Ensure you have these files in an "Assets" folder next to your executable.
        //playerTex_ = gfx_->loadTextureFromFile("Assets/duck-rgba-256.tex");
        //groundTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png");
        gfx_->setAlphaBlend(true); // Enable transparency for PNGs

		resourceManager_->loadTexture("playerSprite_DuckyWucky", "Assets/duck-rgba-256.tex");

        // --- Create Ground Entity ---
        ground_ = createEntity();
        ecs_->platformTag[ground_] = PlatformTag{};
        ecs_->position[ground_] = Position{ {0.0f, -1.0f} };
        ecs_->velocity[ground_] = Velocity{};
        ecs_->rigidBody[ground_] = RigidBody{ {0.2f,0.2} };
        ecs_->rigidBody[ground_].SetMass(0.0f); // A mass of 0 makes it static/immovable.
        ecs_->colliders[ground_] = AABBCollider{ {1.0f, 0.2f} };
        ecs_->rotationFlag[ground_] = RotationFlag{}; //added rotation for ground

        //to test for rotation
        /*ground2_ = createEntity(); 
        ecs_->platformTag[ground2_] = PlatformTag{};
        ecs_->position[ground2_] = Position{ {0.5f, 0.5f} }; 
        ecs_->velocity[ground2_] = Velocity{};
        ecs_->rigidBody[ground2_] = RigidBody{ {0.2f,0.2} };
        ecs_->rigidBody[ground2_].SetMass(0.0f);
        ecs_->colliders[ground2_] = AABBCollider{ {1.0f, 0.2f} };
        ecs_->rotationFlag[ground2_] = RotationFlag{};
        ecs_->rotationFlag[ground2_].currentRotation = 90.0f;*/


        // --- Create Player Entity ---
        PlayerStats stats = fileIO.loadPlayerStats(); // Example stats from file loading
		shared_ptr<Framework::Texture> playerTexture = resourceManager_->get<Texture>("playerSprite_DuckyWucky");
        player_ = createPlayer(*ecs_, stats, { 0.0f, 0.f }, { 0.1f, 0.1f }, playerTexture);

    }

    void GameSystem::Update(float dt) {
        // 'P' key toggles the master pause state.
        if (input_->KeyPressed(InputSystem::Key::P)) {
            physics_->SetPaused(!physics_->IsPaused());
        }

        // 'O' key toggles the step-by-step debug mode.
        if (input_->KeyPressed(InputSystem::Key::O)) {
            physics_->ToggleStepMode();
        }

        // 'I' key executes a single physics step if in step mode.
        if (input_->KeyPressed(InputSystem::Key::I)) {
            physics_->ExecuteNextStep();
        }

        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });

      
        // Render the ground
      /*  if (groundTex_ && ecs_->position.count(ground_)) {
            const auto& pos = ecs_->position.at(ground_).Pos;
            const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
            gfx_->drawSprite(groundTex_, pos, halfSize, 0.0f);
        }*/

        if (groundTex_ && ecs_->position.count(ground_)) {
            const auto& pos = ecs_->position.at(ground_).Pos;
            const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
            float rotation = 0.0f;
            if (ecs_->rotationFlag.count(ground_)) {
                rotation = ecs_->rotationFlag.at(ground_).currentRotation * (M_PI / 180.0f); 
            }

            gfx_->drawSprite(groundTex_, pos, halfSize, rotation);
        }

        // Render the player
        if (ecs_->tex.count(player_) && ecs_->position.count(player_)) {
            const auto& pos = ecs_->position.at(player_).Pos;
            const auto& halfSize = ecs_->colliders.at(player_).HalfSize;
            gfx_->drawSprite(*ecs_->tex[player_].tex, pos, halfSize, 0.0f);
        }

        // For now, we can use the console to see the state.
        if (physics_->IsInStepMode()) {
            
             std::cout << "STEP MODE ACTIVE (Press 'I' to advance)" << std::endl;
        }
        else if (physics_->IsPaused()) {
             std::cout << "PAUSED" << std::endl;
        }

        //RenderColliders();

    } 

    //void GameSystem::RenderColliders() {
    //    // Loop through all entities that have a collider component
    //    for (auto const& [entity, collider] : ecs_->colliders) {
    //        // Ensure the entity also has a position
    //        if (ecs_->position.count(entity)) {
    //            const auto& pos = ecs_->position.at(entity).Pos;
    //            const auto& halfSize = collider.HalfSize;

    //            float rotation = 0.0f;
    //            if (ecs_->rotationFlag.count(entity)) {
    //                rotation = ecs_->rotationFlag.at(entity).currentRotation * (M_PI / 180.0f);
    //            }

    //            AABB_Bounds rotatedBounds = Framework::PhysicsSystem::GetRotatedAABB(pos, halfSize, rotation);



    //            // Determine the render color based on the entity's tag
    //            Color col = { 0.8f, 0.8f, 0.8f, 1.0f }; // Default color is grey
    //            if (ecs_->playerTag.count(entity))   col = { 1.0f, 0.4f, 0.4f, 1.0f }; // Player is red
    //            if (ecs_->platformTag.count(entity)) col = { 0.4f, 1.0f, 0.4f, 1.0f }; // Platform is green

    //            // Calculate the min/max corners and issue the draw call
    //            //gfx_->drawRectangle(
    //            //    { pos.x - halfSize.x, pos.y - halfSize.y }, // Min corner (bottom-left)
    //            //    { pos.x + halfSize.x, pos.y + halfSize.y }, // Max corner (top-right)
    //            //    col
    //            //);

    //            gfx_->drawRectangle(
    //                rotatedBounds.min, // Min corner 
    //                rotatedBounds.max, // Max corner 
    //                col
    //            );
    //        }
    //    }
    //}

    void GameSystem::RenderColliders() {
        for (auto const& [entity, collider] : ecs_->colliders) {
            if (ecs_->position.count(entity)) {
                const auto& pos = ecs_->position.at(entity).Pos;
                const auto& halfSize = collider.HalfSize;

                float rotation = 0.0f;
                if (ecs_->rotationFlag.count(entity)) {
                    rotation = ecs_->rotationFlag.at(entity).currentRotation;
                }

                AABB_Bounds rotatedBounds;
                if (std::abs(rotation) < 0.001f) {
                    rotatedBounds = {
                        {pos.x - halfSize.x, pos.y - halfSize.y},
                        {pos.x + halfSize.x, pos.y + halfSize.y}
                    };
                }
                else {
                    float rotationRad = rotation * M_PI / 180.0f;
                    float cos_r = std::cos(rotationRad);
                    float sin_r = std::sin(rotationRad);

                    Vector2D corners[4] = {
                        {-halfSize.x, -halfSize.y}, {halfSize.x, -halfSize.y},
                        {halfSize.x, halfSize.y}, {-halfSize.x, halfSize.y}
                    };

                    float minX = INFINITY, maxX = -INFINITY;
                    float minY = INFINITY, maxY = -INFINITY;

                    for (int i = 0; i < 4; i++) {
                        float rotatedX = corners[i].x * cos_r - corners[i].y * sin_r + pos.x;
                        float rotatedY = corners[i].x * sin_r + corners[i].y * cos_r + pos.y;

                        minX = std::min(minX, rotatedX);
                        maxX = std::max(maxX, rotatedX);
                        minY = std::min(minY, rotatedY);
                        maxY = std::max(maxY, rotatedY);
                    }
                    rotatedBounds = { {minX, minY}, {maxX, maxY} };
                }

                Color col = { 0.8f, 0.8f, 0.8f, 1.0f }; // Default color is grey
                if (ecs_->playerTag.count(entity))   col = { 1.0f, 0.4f, 0.4f, 1.0f }; // Player is red
                if (ecs_->platformTag.count(entity)) col = { 0.4f, 1.0f, 0.4f, 1.0f }; // Platform is green

                // Draw the rotated bounding box
                gfx_->drawRectangle(
                    rotatedBounds.min, // Min corner 
                    rotatedBounds.max, // Max corner 
                    col
                );
            }
        }
    }

} // namespace Framework
