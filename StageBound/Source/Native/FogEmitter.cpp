/*****************************************************************//**
 * \file	FogEmitter.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author

 *
 * \brief   Script for emitting fog VFX in the level.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "FogEmitter.h"
#include "ScriptRegistry.h"

#include <random>
#include <vector>
#include <string>
#include "GameManager.h"
#include "EntityManager.h"
#include "OwalaTime.h"

REGISTER_SCRIPT(FogEmitter, "FogEmitter")

void FogEmitter::OnCreate() {
    // Initial spawn delay
    nextSpawnTimer = 1.0f;
}

void FogEmitter::OnUpdate() {
    float dt = Time::DeltaTime();
    nextSpawnTimer -= dt;

    // Stop fog VFX logic if current scene is not level scene (eg: pause/options menu)
    const SceneAsset& currentScene = SceneManager::GetInstance().GetActiveScene();
    if (!GameManager::GetInstance().IsLevelScene(currentScene.name)) return;

    EntityManager entityMgr(GameManager::GetInstance().GetECS());

    // Handle Spawning
    if (nextSpawnTimer <= 0.0f) {
        CameraSystem* cameraSys = GameManager::GetInstance().GetSystems().camera_;
        Entity activeCamera = cameraSys->GetActiveCamera();

        if (activeCamera <= 0) return;

        // Get Camera bounds
        float camLeft{}, camRight{}, camBottom{}, camTop{};
        cameraSys->GetViewBounds(activeCamera, camLeft, camRight, camBottom, camTop);

        // Get Map Bounds
        Framework::Camera& camData = cameraSys->GetCamera(activeCamera);

        // Fallback to arbitrary large numbers if map bounds aren't currently enabled
        float mapLeft = camData.useMapBounds ? camData.mapMinX : -10000.0f;
        float mapRight = camData.useMapBounds ? camData.mapMaxX : 10000.0f;
        float mapBottom = camData.useMapBounds ? camData.mapMinY : -10000.0f;
        float mapTop = camData.useMapBounds ? camData.mapMaxY : 10000.0f;

        // Calculate valid Y-spawn range
        float validYTop = std::min(camTop, mapTop);
        float validYBottom = std::max(camBottom, mapBottom);

        // Check if the camera is actually looking at a valid Y segment
        if (validYTop > validYBottom) {
            // Determine which walls are currently in the camera's view
            std::vector<int> visibleWalls;

            // Determine if walls are in view (with 100px padding)
            if (mapLeft >= camLeft - 100.0f && mapLeft <= camRight) {
                visibleWalls.push_back(0); // Left Wall
            }
            if (mapRight <= camRight + 100.0f && mapRight >= camLeft) {
                visibleWalls.push_back(1); // Right Wall
            }

            // Spawn if a wall is visible
            if (!visibleWalls.empty()) {
                // Pick a random visible wall
                int wallIndex = visibleWalls[rand() % visibleWalls.size()];

                // Pick a random Y position within the visible segment
                float range = validYTop - validYBottom;
                float spawnY = validYBottom + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / range));

                // Create Entity
                Entity fogEntity = entityMgr.CreateEntity();

                // Randomly select texture
                std::string textures[3] = { "Fog_01.png", "Fog_02.png", "Fog_03.png" };
                int texIndex = rand() % 3;

                entityMgr.AddComponent(fogEntity, TextureComponentType);
                if (entityMgr.HasComponent(fogEntity, TextureComponentType)) {
                    TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(fogEntity);
                    texture.textureKey = textures[texIndex];
                    texture.HalfSize = fogTextureHalfSize;
                }

                // Setup initial position and velocity
                entityMgr.AddComponent(fogEntity, TransformType);
                entityMgr.AddComponent(fogEntity, VelocityType);

                if (entityMgr.HasComponent(fogEntity, TransformType) && entityMgr.HasComponent(fogEntity, VelocityType)) {
                    if (wallIndex == 0) { // Left Wall
                        Transform& transform = entityMgr.GetComponent<Transform>(fogEntity);
                        transform.position.x = mapLeft - fogTextureHalfSize.x;
                        transform.position.y = spawnY;

                        Velocity& vel = entityMgr.GetComponent<Velocity>(fogEntity);
                        vel.Vel.x = fogVelocity;
                    }
                    else { // Right Wall
                        Transform& transform = entityMgr.GetComponent<Transform>(fogEntity);
                        transform.position.x = mapRight + fogTextureHalfSize.x;
                        transform.position.y = spawnY;

                        Velocity& vel = entityMgr.GetComponent<Velocity>(fogEntity);
                        vel.Vel.x = -fogVelocity;
                    }
                }

                // Setup render layer so it's behind the outer walls
                EntityProperties& props = entityMgr.GetProperties(fogEntity);
                props.name = "FogVFX";
                //props.renderLayer = RenderLayer::Custom;
                //props.customRenderOrder = -10; // Outer walls usually at 0

                // Track the entity for fading
                activeFogs.push_back({ fogEntity, fogLifeSpan, fogLifeSpan });
        
                // Add entity to scene
                SceneManager::GetInstance().AddEntityToScene(currentScene, fogEntity);
            }
        }
        
        // Reset timer to a random interval
        float range = maxSpawnInterval - minSpawnInterval;
        nextSpawnTimer = minSpawnInterval + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / range));
    }

    // Handle Updating & Fading
    for (auto it = activeFogs.begin(); it != activeFogs.end(); ) {
        it->lifeTimer -= dt;

        if (it->lifeTimer <= 0.0f) {
            // Despawn entity
            entityMgr.DestroyEntity(it->entity);
            it = activeFogs.erase(it);
        } else {
            // Calculate fade out (alpha approaches 0 as lifeTimer approaches 0)
            float currentAlpha = it->lifeTimer / it->maxLife;
            TextureComponent& texture = entityMgr.GetComponent<TextureComponent>(it->entity);
            texture.alpha = currentAlpha;

            ++it;
        }
    }
}

void FogEmitter::OnDestroy() {
    activeFogs.clear();
}