/*******************************************************************
 * \file   PlayerSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen (50%)
 * \secondary_author
 *		Lur Pei Hao Stanley (45%)
 *      Sitara Anil (5%)
 * \brief	Contains Implementations of a Player System that
 *			updates the player entity's components.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.   
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "EntityStorage.h"
#include "PlayerSystem.h"
#include <iostream>
#include "GameManager.h"
#include "InputSystem.h" // Include for Input System
#include "PhysicsSystem.h"
#include "FileUtility.h" // Include to write player data on Shutdown
#include "EntityManager.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include "ParticleEffectManager.h"
#include "OwalaTime.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"
#include "SceneManager.h" 
#include "LevelSelection.h"
#include "TransitionController.h"

namespace Framework {
    void PlayerSystem::Initialize() {
        m_deathEventFired = false;

        // Load player stats from config
        {
            rapidjson::Document d;
            if (FileIOSystem::ReadFile(FileIOKeys::PLAYER_CONFIG_FILEPATH, d)) {
                auto SafeReadFloat = [&](const char* key, float& outValue) {
                    if (d.HasMember(key) && d[key].IsNumber())
                        outValue = d[key].GetFloat();
                    else
                        Logger::Get().Error(DebugType::FileIOSystem) << "Invalid or missing '" << key << "' in " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
                };
                SafeReadFloat(FileIOKeys::PLAYER_MV_SPEED,                  m_stats.moveSpeed);
                SafeReadFloat(FileIOKeys::PLAYER_JUMP_HEIGHT,                m_stats.jumpHeight);
                SafeReadFloat(FileIOKeys::PLAYER_INVULNERABILITY_DURATION,   m_stats.invulnerabilityDuration);
                SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_DURATION,         m_stats.knockbackDuration);
                SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_FORCE_X,          m_stats.knockbackForceX);
                SafeReadFloat(FileIOKeys::PLAYER_KNOCKBACK_FORCE_Y,          m_stats.knockbackForceY);
                Logger::Get().Info(DebugType::FileIOSystem) << "Player data read successfully." << std::endl;
            }
            else {
                Logger::Get().Error(DebugType::FileIOSystem) << "Unable to read file: " << FileIOKeys::PLAYER_CONFIG_FILEPATH << std::endl;
            }
        }

        ParticleEffectManager::GetInstance().LoadEffectPresets("Data/ParticleEffects/particleEffects.json");

        MessageBus::Instance().Subscribe<ButtonClickedEvent>([this](const ButtonClickedEvent& event) {
            if (event.action == ButtonAction::RESETPLAYERPOS) {
                EntityManager entityMgr(GameManager::GetInstance().GetECS());

                Entity spawnPtEntity = 0;
                std::vector<Entity> spawnPtEntities = entityMgr.GetAllEntitiesWithComponents(true, TransformType, SpawnPointTagType);
                if (spawnPtEntities.size() > 0) spawnPtEntity = spawnPtEntities[0];

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
                    if (entityMgr.HasComponent(entity, TransformType) && spawnPtEntity > 0 && entityMgr.HasComponent(spawnPtEntity, TransformType)) {
                        Transform& spawnPt = entityMgr.GetComponent<Transform>(spawnPtEntity);
                        entityMgr.GetComponent<Transform>(entity).position = spawnPt.position;
                    }
                    else {
                        Logger::Get().Error(DebugType::PlayerSystem) << "Player spawn point not found!" << std::endl;
                    }
                }
            }
        });
    
        MessageBus::Instance().Subscribe<PlayerDeathEvent>([this](const PlayerDeathEvent& event) {
            if (event.deathAnimationFinished && !m_lvlClearEventFired) {
                if (!GameManager::GetInstance().GetNextScene().empty()) return;

                if (SceneManager::GetInstance().GetActiveScene().name != "RestartMenu") {
                    Logger::Get().Info(DebugType::GameManager) << "Changing to RestartMenu scene" << std::endl;
                    LevelSelection::ResetFade();
                    GameManager::GetInstance().SetOverlayCallback(nullptr);
                    // Set the next scene immediately to block re-entrant calls
                    //GameManager::GetInstance().SetNextScene("RestartMenu");

                    // TELL THE CURTAINS TO CLOSE HERE
                    TransitionController::PlayAllClose([]() {
                        GameManager::GetInstance().InitiateLevelLoad("RestartMenu");
                        });
                }
            }
        });

        MessageBus::Instance().Subscribe<LevelClearEvent>([this](const LevelClearEvent&) {
            if (m_lvlClearEventFired || m_deathEventFired) return;
            if (!GameManager::GetInstance().GetNextScene().empty()) return;

            if (SceneManager::GetInstance().GetActiveScene().name != "GameWinScreen") {
                Logger::Get().Info(DebugType::GameManager) << "Changing to GameWinScreen scene" << std::endl;
                LevelSelection::ResetFade();
                GameManager::GetInstance().SetOverlayCallback(nullptr);
                m_lvlClearEventFired = true;
                // Set the next scene immediately to block re-entrant calls

                // TELL THE CURTAINS TO CLOSE HERE
                TransitionController::PlayAllClose([]() {
                    GameManager::GetInstance().InitiateLevelLoad("GameWinScreen");
                    });
            }
        });

        MessageBus::Instance().Subscribe<LevelStartEvent>([this](const LevelStartEvent&) {
            m_deathEventFired = false;
            m_lvlClearEventFired = false;
        });
    }

    void PlayerSystem::Update() {
        GameManager& gm = GameManager::GetInstance();
        PhysicsSystem& physics_ = *gm.GetSystems().physics_;
        if (physics_.IsPaused()) { return; }

        TimerSystem::Get().Update(Framework::Time::DeltaTime());
        FirebaseManager::Get().Update();


        // In PlayerSystem::Update(), at the beginning of the player loop:

        InputSystem& input_ = *gm.GetSystems().input_;

        EntityManager entityMgr(gm.GetECS()); // To access entity creation/destruction if needed

        UpdateInvulnerability();

        // Skip death checks if level already cleared / already died
        if (!m_lvlClearEventFired && !m_deathEventFired) {
            CheckPlayerDeathFromTraps();
            CheckPlayerDeathFromEnemies();
            HandlePlayerDeath();
            CheckEnemyDeathFromTraps();
        }

        // calculate jump vel based on height and gravity -  v = sqrt(2 * g * h).
        const float gravity = std::abs(physics_.gravity_.y);
        const float jumpVelocity = std::sqrt(2.0f * gravity * m_stats.jumpHeight);

        // Loop through each entity that is in the playerTag map
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            // Tag player entities for viewport clamping if not already tagged.
            if (!entityMgr.HasComponent(entity, ViewportConstrainedTagType)) {
                entityMgr.AddComponent(entity, ViewportConstrainedTag{});
            }

            // Check if Player Entity is active in the scene. Handle player controller if active.
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

            // We need a RigidBody, Velocity, and PlayerState to control the player
            if (entityMgr.HasComponent(entity, VelocityType) && entityMgr.HasComponent(entity, PlayerStateType) && entityMgr.HasComponent(entity, AnimationComponentType)) {
                Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);

                // Get Position
                Vector2D currentPos = { 0,0 };
                if (entityMgr.HasComponent(entity, TransformType)) {
                    currentPos = entityMgr.GetComponent<Transform>(entity).position;
                }

                // --- KNOCKBACK TIMER LOGIC START ---
                if (state.isKnockedBack) {
                    state.knockbackTimer -= Framework::Time::DeltaTime();
                    if (state.knockbackTimer <= 0.0f) {
                        state.isKnockedBack = false;
                    }
                }
                // --- KNOCKBACK TIMER LOGIC END ---

                float currentVelY = vel.Vel.y;
                float prevVelY = state.previousVelocityY; // Get from Component
                float stopThreshold = 0.01f;

                // Logic: 
                // 1. We were falling fast (prev < 0)
                // 2. We stopped vertically (current ~ 0)
                // 3. We are touching the ground (isGrounded)
                if (prevVelY < -stopThreshold &&
                    std::abs(currentVelY) < stopThreshold &&
                    state.isGrounded)
                {
                    // Pass the impact speed (absolute value of previous fall speed)
                    OnPlayerLanded(entity, currentPos);
                }

                // Update the component for the next frame
                state.previousVelocityY = currentVelY;

                // Capture isWalking state before its updated
                bool wasWalking = state.isWalking;

                // Horizontal Movement 
                if (!state.isDead) {
                    float targetVelX = 0.0f;
                    if (!state.isKnockedBack) {
                        if (input_.KeyDown(InputSystem::Key::A)) targetVelX = -m_stats.moveSpeed;
                        if (input_.KeyDown(InputSystem::Key::D)) targetVelX = m_stats.moveSpeed;
                        vel.Vel.x = targetVelX;

				    // Apply Jumping
                    if (state.isGrounded && (input_.KeyPressed(InputSystem::Key::W) || input_.KeyPressed(InputSystem::Key::Space))) {
                        vel.Vel.y = jumpVelocity; // Apply the calculated upward velocity
                        state.isWalking = false;

                            //messageBus
                            MessageBus::Instance().Publish(PlayerJumpedEvent{ entity });
                            MessageBus::Instance().Publish(PlaySoundEvent{ "Player_Jump", AudioType::SFX, 0.3f });
                        }

                        // Sets the direction the player is facing based on movement
                        if (targetVelX > 0.01f) {
                            state.isFacingRight = true;
                        }
                        else if (targetVelX < -0.01f) {
                            state.isFacingRight = false;
                        }
                    }
                    /*else {
                        vel.Vel.x = 0.0f;
                    }*/

				    // Sets the direction the player is facing based on movement
                    if (targetVelX > 0.01f) {
                        state.isFacingRight = true;
                        state.isWalking = true;
                    }
                    else if (targetVelX < -0.01f) {
                        state.isFacingRight = false;
                        state.isWalking = true;
                    }
                    else {
                        state.isWalking = false;
                    }
                }
                else {
                    // If player is dead
                    vel.Vel.x = 0.0f;
                }

                // Handle invulnerability overlap separation
                if (state.isHurt && entityMgr.HasComponent(entity, TransformType)) {
                    SeparatePlayerFromDynamicObjects(entity);
                }

                if (state.isDead) {
                    vel.Vel.x = 0.0f;
                    if (state.isGrounded) {
                        if (entityMgr.HasComponent(entity, RigidBodyType)) {
                            RigidBody& rb = entityMgr.GetComponent<RigidBody>(entity);
                            rb.InverseMass = 0.0f;
                        }
                        vel.Vel.y = 0.0f;
                    }
                }

                // ========== ANIMATION STATE HANDLING ==========
                std::string animState = "IDLE";
                if (state.isDead) {
                    animState = "DEATH";
                    animationComponent.isLooping = false;
                }
                else if (state.isHurt) {
                    animState = "HURT";
                }
                else if (!state.isGrounded) {
                    if (vel.Vel.y > 0.01f) {
                        animState = "JUMPING";
                    }
                    else {
                        animState = "FALLING";
                    }
                }
                else {
                    if (std::abs(vel.Vel.x) > 0.01f) {
                        animState = "RUNNING";
                    }
                    else {
                        animState = "IDLE";
                    }
                }

                // Set the animation state
                animationComponent.currentState = animState;
            }
        }
    }

        void PlayerSystem::UpdateInvulnerability() {
            EntityManager entityMgr(GameManager::GetInstance().GetECS());

            for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
                if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
                if (!entityMgr.HasComponent(entity, PlayerStateType)) continue;

                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                EntityProperties& props = entityMgr.GetProperties(entity);

                if (state.isHurt) {
                    state.invulnerabilityTimer += Framework::Time::DeltaTime();

                    if (state.invulnerabilityTimer >= m_stats.invulnerabilityDuration) {
                        // End invulnerability
                        state.isHurt = false;

                        state.invulnerabilityTimer = 0.0f;

                        props.layer = CollisionLayer::Player;

                        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player invulnerability ended. Lives remaining: " << state.currentLives << std::endl;
                    }
                }
            }
        }

    void PlayerSystem::SeparatePlayerFromDynamicObjects(Entity playerEntity) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, TransformType) || 
            !entityMgr.HasComponent(playerEntity, AABBColliderType) ||
            !entityMgr.HasComponent(playerEntity, PlayerStateType)) {
            return;
        }

        Transform& playerTransform = entityMgr.GetComponent<Transform>(playerEntity);
        const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);

        // Only start separating in the last 20% of invulnerability time
        float separationStartTime = m_stats.invulnerabilityDuration * 0.8f;
        if (playerState.invulnerabilityTimer < separationStartTime) {
            return; // Too early, don't separate yet
        }

        // Calculate separation strength (0 to 1, where 1 is end of invulnerability)
        float separationProgress = (playerState.invulnerabilityTimer - separationStartTime) / (m_stats.invulnerabilityDuration - separationStartTime);
        float separationSpeed = 50.0f * Framework::Time::DeltaTime() * separationProgress; // Gradual separation

        AABB_Bounds playerBounds = {
            {playerTransform.position.x - playerHalfSize.x, playerTransform.position.y - playerHalfSize.y},
            {playerTransform.position.x + playerHalfSize.x, playerTransform.position.y + playerHalfSize.y}
        };

        // Check all dynamic trap objects
        for (const Entity& dynamicEntity : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
            if (!entityMgr.HasComponent(dynamicEntity, RigidBodyType) ||
                !entityMgr.HasComponent(dynamicEntity, TransformType) ||
                !entityMgr.HasComponent(dynamicEntity, AABBColliderType)) {
                continue;
            }

            const Vector2D& objPos = entityMgr.GetComponent<Transform>(dynamicEntity).position;
            const Vector2D& objHalfSize = entityMgr.GetComponent<AABBCollider>(dynamicEntity).HalfSize;

            float objRotation = 0.0f;
            if (entityMgr.HasComponent(dynamicEntity, RotationFlagType)) {
                objRotation = entityMgr.GetComponent<RotationFlag>(dynamicEntity).currentRotation;
            }
            AABB_Bounds objBounds = PhysicsSystem::GetRotatedAABB(objPos, objHalfSize, objRotation);

            // Check if overlapping
            if (CollisionIntersection_RectRect_Static(playerBounds, objBounds)) {
                // Calculate penetration on each axis
                float overlapLeft = playerBounds.max.x - objBounds.min.x;
                float overlapRight = objBounds.max.x - playerBounds.min.x;
                float overlapTop = playerBounds.max.y - objBounds.min.y;
                float overlapBottom = objBounds.max.y - playerBounds.min.y;

                // Find minimum overlap
                float minOverlapX = std::min(overlapLeft, overlapRight);
                float minOverlapY = std::min(overlapTop, overlapBottom);

                // Separate along the axis with least penetration (gentler)
                if (minOverlapX < minOverlapY) {
                    // Separate horizontally
                    if (overlapLeft < overlapRight) {
                        playerTransform.position.x -= separationSpeed;
                    } else {
                        playerTransform.position.x += separationSpeed;
                    }
                } else {
                    // Separate vertically
                    if (overlapTop < overlapBottom) {
                        playerTransform.position.y -= separationSpeed;
                    } else {
                        playerTransform.position.y += separationSpeed;
                    }
                }

                // Update bounds for next object check
                playerBounds = {
                    {playerTransform.position.x - playerHalfSize.x, playerTransform.position.y - playerHalfSize.y},
                    {playerTransform.position.x + playerHalfSize.x, playerTransform.position.y + playerHalfSize.y}
                };
            }
        }
    }


    // ========== PLAYER DEATH LOGIC ==========

    void PlayerSystem::CheckPlayerDeathFromTraps() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        Entity playerEntity = 0;
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (entityMgr.HasComponent(entity, LoadedTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);

        if (playerState.isDead || playerState.isHurt || playerState.isKnockedBack) return;

        // Loop through all trap platforms
        for (const Entity& trapEntity : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
            TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

            // Only check traps that are currently falling
            if (!trapComp.isTriggered && !trapComp.hasFallen) continue;

            const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& trapPos = entityMgr.GetComponent<Transform>(trapEntity).position;
            const Vector2D& trapHalfSize = entityMgr.GetComponent<AABBCollider>(trapEntity).HalfSize;

            AABB_Bounds playerBounds = {
                {playerPos.x - playerHalfSize.x, playerPos.y - playerHalfSize.y},
                {playerPos.x + playerHalfSize.x, playerPos.y + playerHalfSize.y}
            };

            float trapRotation = 0.0f;
            if (entityMgr.HasComponent(trapEntity, RotationFlagType)) {
                trapRotation = entityMgr.GetComponent<RotationFlag>(trapEntity).currentRotation;
            }
            AABB_Bounds trapBounds = PhysicsSystem::GetRotatedAABB(trapPos, trapHalfSize, trapRotation);

            // Check Collision
            if (CollisionIntersection_RectRect_Static(playerBounds, trapBounds)) {
                // Check if trap has velocity component to determine if it's falling
                bool isTrapFalling = false;
                float trapVelocityY = 0.0f;
                
                if (entityMgr.HasComponent(trapEntity, VelocityType)) {
                    trapVelocityY = entityMgr.GetComponent<Velocity>(trapEntity).Vel.y;
                    // Trap is falling if it has significant downward velocity
                    isTrapFalling = trapVelocityY < 0.0f; // Threshold for falling
                }

                // Only damage if trap is falling AND hitting player from above
                float trapBottom = trapBounds.min.y;
                float playerTop = playerBounds.max.y;

				// Small tolerance to ensure "from above"
                bool isHittingFromAbove = (trapBottom <= playerTop + 20.0f);

                if (isTrapFalling && isHittingFromAbove) {
                    DamagePlayer(playerEntity, "falling trap platform", trapPos);
                    break;
                }
            }
        }
    }

    void PlayerSystem::CheckPlayerDeathFromEnemies() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        Entity playerEntity{};
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (entityMgr.HasComponent(entity, LoadedTagType) && entityMgr.HasComponent(entity, PlayerStateType)) {
                playerEntity = entity;
                break;
            }
        }

        if (playerEntity == 0) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead || playerState.isHurt) return;

        for (const Entity& enemy : entityMgr.GetAllEntitiesWithComponent(true, EnemyTagType)) {
            if (!entityMgr.HasComponent(enemy, TransformType) ||
                !entityMgr.HasComponent(enemy, AABBColliderType) ||
                !entityMgr.HasComponent(playerEntity, TransformType) ||
                !entityMgr.HasComponent(playerEntity, AABBColliderType)) {
                continue;
            }

            const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
            const Vector2D& playerHalfSize = entityMgr.GetComponent<AABBCollider>(playerEntity).HalfSize;
            const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemy).position;
            const Vector2D& enemyHalfSize = entityMgr.GetComponent<AABBCollider>(enemy).HalfSize;

            AABB_Bounds playerBounds = {
                {playerPos.x - playerHalfSize.x, playerPos.y - playerHalfSize.y},
                {playerPos.x + playerHalfSize.x, playerPos.y + playerHalfSize.y}
            };

            AABB_Bounds enemyBounds = {
                {enemyPos.x - enemyHalfSize.x, enemyPos.y - enemyHalfSize.y},
                {enemyPos.x + enemyHalfSize.x, enemyPos.y + enemyHalfSize.y}
            };

            if (CollisionIntersection_RectRect_Static(playerBounds, enemyBounds)) {
                std::string enemyType = "enemy";
                if (entityMgr.HasComponent(enemy, FSMComponentType)) {
                    enemyType = "bird enemy";
                }

                DamagePlayer(playerEntity, enemyType, enemyPos);
                break;
            }
        }
    }

    void PlayerSystem::DamagePlayer(Entity playerEntity, const std::string& damageSource, const Vector2D& sourcePos) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, PlayerStateType)) return;

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);

        if (playerState.isHurt || playerState.isDead) return;

        playerState.currentLives--;
        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player hit by " << damageSource << "! Lives: " << playerState.currentLives << std::endl;

        if (playerState.currentLives <= 0) {
            KillPlayer(playerEntity, damageSource);
        }
        else {
            playerState.isHurt = true;
            playerState.invulnerabilityTimer = 0.0f;

            EntityProperties& props = entityMgr.GetProperties(playerEntity);
            props.layer = CollisionLayer::Invulnrabilty;

            if (entityMgr.HasComponent(playerEntity, TransformType) && entityMgr.HasComponent(playerEntity, VelocityType)) {
                Vector2D playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
                Velocity& vel = entityMgr.GetComponent<Velocity>(playerEntity);
                float directionX = (sourcePos.x > playerPos.x) ? -1.0f : 1.0f;

                // Apply knockback impulse
                vel.Vel.x = directionX * m_stats.knockbackForceX;
                vel.Vel.y = m_stats.knockbackForceY;

                playerState.isKnockedBack = true;
                playerState.knockbackTimer = m_stats.knockbackDuration;
                playerState.isGrounded = false;
                playerState.isFacingRight = (directionX < 0);

                Logger::Get().Info(DebugType::PlayerSystem)
                    << "Knockback applied: X=" << vel.Vel.x << ", Y=" << vel.Vel.y
                    << ", directionX=" << directionX << std::endl;
            }

            if (entityMgr.HasComponent(playerEntity, TransformType)) {
                const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
                MessageBus::Instance().Publish(VFXTriggerEvent{ "PlayerHitEvent", true, playerPos });
            }

            MessageBus::Instance().Publish(PlayerDamagedEvent{ playerEntity, playerState.currentLives });
            MessageBus::Instance().Publish(PlaySoundEvent{ "Player_Hurt", AudioType::SFX, 0.3f });
        }
    }


    bool PlayerSystem::IsPlayerInAttackRange(Entity playerEntity, Entity enemyEntity, float attackRadius) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, TransformType) ||
            !entityMgr.HasComponent(enemyEntity, TransformType)) {
            return false;
        }

        const Vector2D& playerPos = entityMgr.GetComponent<Transform>(playerEntity).position;
        const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemyEntity).position;

        float distance = playerPos.Distance(enemyPos);
        return distance <= attackRadius;
    }

    void PlayerSystem::KillPlayer(Entity playerEntity, const std::string& killerType) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(playerEntity, PlayerStateType)) {
            return;
        }

        PlayerState& playerState = entityMgr.GetComponent<PlayerState>(playerEntity);
        if (playerState.isDead) return;

        playerState.isDead = true;
        playerState.currentLives = 0;

        /*if (entityMgr.HasComponent(playerEntity, VelocityType)) {
            entityMgr.GetComponent<Velocity>(playerEntity).Vel = Vector2D::Zero();
        }
        if (entityMgr.HasComponent(playerEntity, RigidBodyType)) {
            entityMgr.GetComponent<RigidBody>(playerEntity).SetMass(0.0f);
        }*/
      

        if (entityMgr.HasComponent(playerEntity, VelocityType)) {
            Velocity& vel = entityMgr.GetComponent<Velocity>(playerEntity);
            vel.Vel.x = 0.0f;
        }
        if (entityMgr.HasComponent(playerEntity, AnimationComponentType)) {
            entityMgr.GetComponent<AnimationComponent>(playerEntity).currentState = "DEATH";
        }
        MessageBus::Instance().Publish(PlayerDeathEvent{ playerEntity, false });
        MessageBus::Instance().Publish(PlaySoundEvent{ "Player_Death", AudioType::SFX, 0.3f });

        Framework::Logger::Get().Info(DebugType::PlayerSystem) << "Player killed by " << killerType << "!" << std::endl;
    }

    void PlayerSystem::CheckEnemyDeathFromTraps() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());
        //float dt = Time::DeltaTime();

        for (const Entity& enemyEntity : entityMgr.GetAllEntitiesWithComponent(true, FSMComponentType)) {
            if (!entityMgr.HasComponent(enemyEntity, EnemyTagType) ||
                !entityMgr.HasComponent(enemyEntity, TransformType) ||
                !entityMgr.HasComponent(enemyEntity, AABBColliderType)) {
                continue;
            }

            FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(enemyEntity);
            if (fsmComp.fsm.GetCurrentState() == "Death") continue;

            // Check all trap platforms
            for (const Entity& trapEntity : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
                TrapPlatformComponent& trapComp = entityMgr.GetComponent<TrapPlatformComponent>(trapEntity);

                // Only check falling traps
                if (!trapComp.isTriggered && !trapComp.hasFallen) continue;

                const Vector2D& enemyPos = entityMgr.GetComponent<Transform>(enemyEntity).position;
                const Vector2D& enemyHalfSize = entityMgr.GetComponent<AABBCollider>(enemyEntity).HalfSize;
                const Vector2D& trapPos = entityMgr.GetComponent<Transform>(trapEntity).position;
                const Vector2D& trapHalfSize = entityMgr.GetComponent<AABBCollider>(trapEntity).HalfSize;

                Vector2D trapVel = { 0.0f, 0.0f };
                if (entityMgr.HasComponent(trapEntity, VelocityType)) {
                    trapVel = entityMgr.GetComponent<Velocity>(trapEntity).Vel;
                }

                Vector2D enemyVel = { 0.0f, 0.0f };
                if (entityMgr.HasComponent(enemyEntity, VelocityType)) {
                    enemyVel = entityMgr.GetComponent<Velocity>(enemyEntity).Vel;
                }

                // Enemy bounds
                AABB_Bounds enemyBounds = {
                    {enemyPos.x - enemyHalfSize.x, enemyPos.y - enemyHalfSize.y},
                    {enemyPos.x + enemyHalfSize.x, enemyPos.y + enemyHalfSize.y}
                };

                float trapRotation = 0.0f;
                if (entityMgr.HasComponent(trapEntity, RotationFlagType)) {
                    trapRotation = entityMgr.GetComponent<RotationFlag>(trapEntity).currentRotation;
                }
                AABB_Bounds trapBounds = PhysicsSystem::GetRotatedAABB(trapPos, trapHalfSize, trapRotation);

                // Physics might have resolved the overlap, so expand trap bounds slightly 
                // to detect "touching" contact.
                float tolerance = 2.0f;
                AABB_Bounds expandedTrapBounds = trapBounds;
                expandedTrapBounds.min.x -= tolerance;
                expandedTrapBounds.max.x += tolerance;
                expandedTrapBounds.min.y -= tolerance;
                expandedTrapBounds.max.y += tolerance;

                // 3. Intersection Check
                if (CollisionIntersection_RectRect_Static(enemyBounds, expandedTrapBounds)) {

                    // --- NEW LOGIC STARTS HERE ---

                    // Calculate the actual overlap dimensions
                    float overlapMinX = std::max(enemyBounds.min.x, expandedTrapBounds.min.x);
                    float overlapMaxX = std::min(enemyBounds.max.x, expandedTrapBounds.max.x);
                    float overlapMinY = std::max(enemyBounds.min.y, expandedTrapBounds.min.y);
                    float overlapMaxY = std::min(enemyBounds.max.y, expandedTrapBounds.max.y);

                    float overlapWidth = std::max(0.0f, overlapMaxX - overlapMinX);
                    float overlapHeight = std::max(0.0f, overlapMaxY - overlapMinY);

                    // A crushing hit is mostly horizontal surface contact (Vertical Penetration < Horizontal Width)
                    bool isVerticalHit = overlapHeight < overlapWidth;

                    // Ensure the trap is physically above the enemy
                    bool isTrapAbove = trapPos.y > enemyPos.y;

                    // Kill ONLY if:
                    // 1. It's a vertical collision (not a side swipe)
                    // 2. The trap is above the bird
                    // 3. The trap is in the falling state
                    if (isVerticalHit && isTrapAbove && trapComp.hasFallen) {
                        TriggerEnemyDeath(enemyEntity, "falling trap platform");
                        break;
                    }
                }
            }
        }
    }

    void PlayerSystem::TriggerEnemyDeath(Entity enemyEntity, const std::string& killerType) {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        if (!entityMgr.HasComponent(enemyEntity, FSMComponentType)) {
            return;
        }

        FSMComponent& fsmComp = entityMgr.GetComponent<FSMComponent>(enemyEntity);

        // Transition to death state
        fsmComp.fsm.ChangeState(enemyEntity, GameManager::GetInstance().GetECS(), "Death");

        Framework::Logger::Get().Info(DebugType::PlayerSystem)  << "Enemy killed by " << killerType << "!" << std::endl;
    }

    void PlayerSystem::Shutdown() {
    }

	// Publish particle burst event when player lands
    void PlayerSystem::OnPlayerLanded(Entity player, const Vector2D& position) {
		(void)player; // Unused parameter
        MessageBus::Instance().Publish(PlayerLandEvent());
        MessageBus::Instance().Publish(PlaySoundEvent{ "Player_Land", AudioType::SFX, 0.3f });
        // Publish VFX Trigger Event for Player Landed, with position override true, and the player's position to tell the event where to spawn the VFX
        MessageBus::Instance().Publish(VFXTriggerEvent{ "PlayerLandedEvent", true, position });
    }

    void PlayerSystem::HandlePlayerDeath() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, PlayerTagType)) {
            if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;

            if (entityMgr.HasComponent(entity, PlayerStateType)) {
                PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);

				// Reset the death event fired flag if player is not dead anymore (e.g. after respawn)
                if (!state.isDead) {
                    m_deathEventFired = false;
                }

                if (state.isDead && entityMgr.HasComponent(entity, AnimationComponentType)) {
                    AnimationComponent& anim = entityMgr.GetComponent<AnimationComponent>(entity);

                    if (anim.currentState == "DEATH" && !anim.isPlaying) {
                        // Only publish the event once!
                        if (!m_deathEventFired) {
                            MessageBus::Instance().Publish(PlayerDeathEvent{ entity, true }); // animation finished
                            m_deathEventFired = true; // Lock it so it doesn't fire again
                        }
                    }
                }
            }
        }
    }

} // namespace Framework
