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
#include "SceneManager.h"
#include <iostream>

// Test Data
const std::string testScene1 = "TestScene1";
const std::string testScene2 = "TestScene2";

namespace Framework {
    void TestLevelsSetup(EntityStorage& ecs, GameSystem& gameSystem) {
        // Initialize Level 1 - This is the original Level
        // --- Create Ground Entity ---
        Entity platform = createEntity();
        ecs.platformTag[platform] = PlatformTag{};
        ecs.position[platform] = Position{ {0.0f, -0.7f} };
        ecs.velocity[platform] = Velocity{};
        ecs.rigidBody[platform] = RigidBody{ {0.2f,0.2} };
        ecs.rigidBody[platform].SetMass(0.0f); // A mass of 0 makes it static/immovable.
        ecs.colliders[platform] = AABBCollider{ {1.0f, 0.2f} };

        // Add Entity to Scene via Helper Functions
        SceneManager::GetInstance().AddEntityToScene(testScene1, platform);

        // Instantiate Level 2
        std::vector<Position> positions = { {{-0.5f, -0.5f}}, {{0.5f, -0.5f}}, {{0.0f, 0.0f}} };
        std::vector<AABBCollider> colliders = { {{1.0f, 0.2f}}, {{1.0f, 0.2f}}, {{1.0f, 0.2f}} };

        for (size_t i = 0; i < positions.size(); ++i) {
            platform = createEntity();
            ecs.position[platform] = positions[i];
            ecs.velocity[platform] = Velocity{};
            ecs.rigidBody[platform] = RigidBody{ {0.2f,0.2} };
            ecs.rigidBody[platform].SetMass(0.0f); // A mass of 0 makes it static/immovable.
            ecs.colliders[platform] = colliders[i];

            // Add Entity to scene manually instead
			ecs.sceneOwned[platform] = SceneOwned{ testScene2 };
        }
    }

    void GameSystem::Initialize() {
        // --- Load Assets ---
        // Ensure you have these files in an "Assets" folder next to your executable.
        playerTex_ = gfx_->loadTextureFromFile("Assets/duck-rgba-256.tex");
        groundTex_ = gfx_->loadTextureFromFile("Assets/Ronald.png");
        gfx_->setAlphaBlend(true); // Enable transparency for PNGs


        // --- Prepare Levels ---
        TestLevelsSetup(*ecs_, *this);

        // --- Create Player Entity ---
        PlayerStats stats = { 3.0f }; // Example stats from file loading
        player_ = createPlayer(*ecs_, stats, { 0.0f, 0.f }, { 0.1f, 0.1f });
        SceneManager::GetInstance().AddEntityToScene(testScene1, player_);
        SceneManager::GetInstance().SetEntityAsPersistent(player_, true); // Player persists across scenes

        // --- Load Level 1 ---
        SceneManager::GetInstance().LoadScene(testScene1); // Start with Scene 1
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

        if (input_->KeyPressed(InputSystem::Key::N)) {
            std::string nextScene = SceneManager::GetInstance().IsSceneLoaded(testScene1) ? testScene2 : testScene1;
            SceneManager::GetInstance().LoadScene(nextScene);
            ecs_->position[player_].Pos = { 0.0f, 0.0f }; // Reset player position on scene change
        }

        if (input_->KeyPressed(InputSystem::Key::E)) {
			SceneManager::GetInstance().PrintCurrentSceneInfo();
		}

        if (input_->KeyPressed(InputSystem::Key::R)) {
            SceneManager::GetInstance().PrintAllScenesInfo();
        }

        gfx_->beginFrame({ 0.08f, 0.10f, 0.14f, 1.0f });


        // Render the platforms
        for (auto const& [entity, activeTag] : ecs_->activeTag) {
            if (groundTex_ && (ecs_->position.find(entity) != ecs_->position.end())) {
                const auto& pos = ecs_->position.at(ground_).Pos;
                const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
                gfx_->drawSprite(groundTex_, pos, halfSize, 0.0f);
            }
        }

        //if (groundTex_ && ecs_->position.count(ground_)) {
        //    const auto& pos = ecs_->position.at(ground_).Pos;
        //    const auto& halfSize = ecs_->colliders.at(ground_).HalfSize;
        //    gfx_->drawSprite(groundTex_, pos, halfSize, 0.0f);
        //}

        // Render the player
        if (playerTex_ && ecs_->position.count(player_)) {
            const auto& pos = ecs_->position.at(player_).Pos;
            const auto& halfSize = ecs_->colliders.at(player_).HalfSize;
            gfx_->drawSprite(playerTex_, pos, halfSize, 0.0f);
        }
    }

    void GameSystem::RenderColliders() {
        // Loop through all entities that have a activeTag component
        for (auto const& [entity, activeTag] : ecs_->activeTag) {
            // Ensure the entity also has a position & collider component
            if ((ecs_->position.find(entity) != ecs_->position.end()) && (ecs_->colliders.find(entity) != ecs_->colliders.end())) {
                const auto& collider = ecs_->colliders.at(entity);
                const auto& pos = ecs_->position.at(entity).Pos;
                const auto& halfSize = collider.HalfSize;

                // Determine the render color based on the entity's tag
                Color col = { 0.8f, 0.8f, 0.8f, 1.0f }; // Default color is grey
                if (ecs_->playerTag.find(entity) != ecs_->playerTag.end()) col = { 1.0f, 0.4f, 0.4f, 1.0f }; // Player is red
                else if (ecs_->platformTag.find(entity) != ecs_->platformTag.end()) col = { 0.4f, 1.0f, 0.4f, 1.0f }; // Platform is green

                // Calculate the min/max corners and issue the draw call
                gfx_->drawRectangle(
                    { pos.x - halfSize.x, pos.y - halfSize.y }, // Min corner (bottom-left)
                    { pos.x + halfSize.x, pos.y + halfSize.y }, // Max corner (top-right)
                    col
                );
            }
        }
    }

    //void GameSystem::RenderColliders() {
    //    // Loop through all entities that have a collider component
    //    for (auto const& [entity, collider] : ecs_->colliders) {
    //        // Ensure the entity also has a position
    //        if (ecs_->position.count(entity)) {
    //            const auto& pos = ecs_->position.at(entity).Pos;
    //            const auto& halfSize = collider.HalfSize;

    //            // Determine the render color based on the entity's tag
    //            Color col = { 0.8f, 0.8f, 0.8f, 1.0f }; // Default color is grey
    //            if (ecs_->playerTag.count(entity))   col = { 1.0f, 0.4f, 0.4f, 1.0f }; // Player is red
    //            if (ecs_->platformTag.count(entity)) col = { 0.4f, 1.0f, 0.4f, 1.0f }; // Platform is green

    //            // Calculate the min/max corners and issue the draw call
    //            gfx_->drawRectangle(
    //                { pos.x - halfSize.x, pos.y - halfSize.y }, // Min corner (bottom-left)
    //                { pos.x + halfSize.x, pos.y + halfSize.y }, // Max corner (top-right)
    //                col
    //            );
    //        }
    //    }
    //}

} // namespace Framework
