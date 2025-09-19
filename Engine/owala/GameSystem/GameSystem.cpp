#include "GameSystem.h"
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include "Player.h"
#include "Platform.h"
#include "EntityStorage.h"
#include "RotationSystem.h"
#include "Matrix3x3.h"
#include "AudioSystem.h"

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

        // --- Create Ground Entity ---
		std::string platformTexID = "RonaldSprite_Platform";
		ground_ = createPlatform(*ecs_, { 0.0f, -0.5f }, { 1.0f, 0.2f }, platformTexID);

        // --- Create Player Entity ---
		FileIOSystem fileIO;
        PlayerStats stats = fileIO.LoadPlayerStats(); // Example stats from file loading

        std::string playerDuckID = "playerSprite_DuckyWucky";
        player_ = createPlayer(*ecs_, stats, { 0.1f, 0.1f }, playerDuckID);

        // ---- AUDIO ----
        // Add AudioSystem BEFORE GameSystem in main.cpp so this is valid now.
        // BGM: loop + stream (e.g., WAV/OGG/MP3). Path must include extension.
        bool ok_bgm = audio_->loadSound(
            "bgm",
            "../Assets/siren.ogg",   // <--- ensure this file exists
            /*loop*/ true,
            /*stream*/ true
        );
        if (!ok_bgm) {
            std::fprintf(stderr, "[Audio] Failed to load bgm\n");
        }
        audio_->play("bgm", /*volume*/0.35f, /*paused*/false);
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

        if (ecs_->tex.count(ground_) && ecs_->position.count(ground_)) {
            const auto& pos = ecs_->position.at(ground_).Pos;
            const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
            float rotation = 0.0f;
            if (ecs_->rotationFlag.count(ground_)) {
                rotation = ecs_->rotationFlag.at(ground_).currentRotation * (M_PI / 180.0f); 
            }

			//Use the textureID to get the actual texture from the resource manager
			std::string textureID = ecs_->tex[ground_].textureKey;
			Texture texture = resourceManager_->get<Texture>(textureID);
            gfx_->drawSprite(texture, pos, halfSize, rotation);
        }

        // Render the player
        if (ecs_->tex.count(player_) && ecs_->position.count(player_)) {
            const auto& pos = ecs_->position.at(player_).Pos;
            const auto& halfSize = ecs_->colliders.at(player_).HalfSize;


			std::string textureID = ecs_->tex[player_].textureKey;
			//Use the textureID to get the actual texture from the resource manager
			Texture texture = resourceManager_->get<Texture>(textureID);
            gfx_->drawSprite(texture, pos, halfSize, 0.0f);
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
