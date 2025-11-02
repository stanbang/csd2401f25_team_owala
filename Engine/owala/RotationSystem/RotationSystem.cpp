// RotationSystem.cpp
#include "RotationSystem.h"
#include "Transformation.h"
#include "EntityManager.h"
#include <cmath>
#include <iostream>
#include "OwalaTime.h"

namespace Framework {
    /*!
    @brief      Constructs the rotation system with references to required
                subsystems
    @param      ecs Reference to entity storage
    @param      input Reference to input system for key detection
    @param      physics Reference to physics system for pause control
    @return     N/A
*//*______________________________________________________________*/

    RotationSystem::RotationSystem(EntityStorage& ecs, InputSystem& input, PhysicsSystem& physics) : ecs_(&ecs), input_(&input), 
        physics_(&physics), globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false), 
        rotationCooldown(0.0f), rotationCooldownDuration(0.5f) {
    }
    /*!
    @brief      Initializes the rotation system (currently empty)
    @param      void
    @return     void
*//*______________________________________________________________*/
    void RotationSystem::Initialize() {
    }
    /*!
    @brief      Updates the global rotation state, handling input for rotation
                triggers and smoothly interpolating between rotation angles.
                Transforms all entities with RotationFlag components.
    @param      dt Delta time in seconds since last frame
    @return     void
*//*______________________________________________________________*/
    void RotationSystem::Update() {
        EntityManager entityMgr(*ecs_);

        // Update cooldown timer
        if (rotationCooldown > 0.0f) {
            rotationCooldown -= Framework::Time::DeltaTime();
            if (rotationCooldown < 0.0f) {
                rotationCooldown = 0.0f;
            }
        }

        if (!isGlobalRotating && rotationCooldown <= 0.0f) {
            if (input_->KeyPressed(InputSystem::Key::E)) {
                targetGlobalRotation -= 90.0f; // Rotate clockwise
                physics_->SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                //std::cout << "Rotating world clockwise 90 degrees" << std::endl;
              
            }

            if (input_->KeyPressed(InputSystem::Key::Q)) {
                targetGlobalRotation += 90.0f; // Rotate counterclockwise
                physics_->SetPaused(true);
                isGlobalRotating = true;
                rotationCooldown = rotationCooldownDuration;
                //std::cout << "Rotating world counterclockwise 90 degrees" << std::endl;
            }
        }

        if (isGlobalRotating) {
            float rotationSpeed = 270.0f;
            float rotationThisFrame = rotationSpeed * Framework::Time::DeltaTime();

            float angleDifference = targetGlobalRotation - globalRotation;

            if (angleDifference > 180.0f) {
                angleDifference -= 360.0f;
            }
            if (angleDifference < -180.0f) {
                angleDifference += 360.0f;
            }

            float rotationIncrement = 0.0f;

            if (std::abs(angleDifference) > rotationThisFrame) {
                if (angleDifference > 0) {
                    rotationIncrement = rotationThisFrame;
                    globalRotation += rotationThisFrame;
                }
                else {
                    rotationIncrement = -rotationThisFrame;
                    globalRotation -= rotationThisFrame;
                }
            }
            else {
                rotationIncrement = angleDifference;
                globalRotation = targetGlobalRotation;
                isGlobalRotating = false;
                physics_->SetPaused(false);
                //std::cout << "Finished world rotation!" << std::endl;
            }

            if (std::abs(rotationIncrement) > 0.001f) {
                Matrix3x3 rotationMat = Transformation::CreateRotationMatrix(rotationIncrement);

                // Rotate the global gravity vector
               // physics_->gravity_ = rotationMat * physics_->gravity_;

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
                    if (entityMgr.HasComponent(entity, PositionType) && entityMgr.HasComponent(entity, ActiveTagType)) {
                        Vector2D originalPos = entityMgr.GetComponent<Position>(entity).Pos;
                        entityMgr.GetComponent<Position>(entity).Pos = rotationMat * originalPos;
                    }
                }

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
                    if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;
                    PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                    state.isGrounded = true;
                }

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
                    if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;

                    RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    if (entityMgr.HasComponent(entity, PlayerTagType)) {
                        continue;
                    }
                    rotationFlag.currentRotation += rotationIncrement;
                    rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
                }

               
            }
        }
    }
    /*!
    @brief      Shuts down the rotation system
    @param      void
    @return     void
*//*______________________________________________________________*/
    void RotationSystem::Shutdown() {
    }
}