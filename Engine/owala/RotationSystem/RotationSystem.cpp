// RotationSystem.cpp
#include "RotationSystem.h"
#include "Transformation.h"
#include "EntityManager.h"
#include <cmath>
#include <iostream>

namespace Framework {
    RotationSystem::RotationSystem(EntityStorage& ecs, InputSystem& input, PhysicsSystem& physics)
        : ecs_(&ecs), input_(&input), physics_(&physics), globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false) {
    }

    void RotationSystem::Initialize() {
    }

    void RotationSystem::Update(float dt) {
		EntityManager entityMgr(*ecs_);

        if (input_->KeyPressed(InputSystem::Key::E)) {
            targetGlobalRotation -= 90.0f; // Rotate clockwise
            physics_->SetPaused(true);
            isGlobalRotating = true;
            //std::cout << "Rotating world clockwise 90 degrees" << std::endl;
        }

        if (input_->KeyPressed(InputSystem::Key::Q)) {
            targetGlobalRotation += 90.0f; // Rotate counterclockwise
            physics_->SetPaused(true);
            isGlobalRotating = true;
            //std::cout << "Rotating world counterclockwise 90 degrees" << std::endl;
        }

        if (isGlobalRotating) {
            float rotationSpeed = 270.0f;
            float rotationThisFrame = rotationSpeed * dt;

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

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
                    RotationFlag& rotationFlag = entityMgr.GetComponent<RotationFlag>(entity);
                    if (entityMgr.HasComponent(entity, PositionType)) {
                        Vector2D originalPos = entityMgr.GetComponent<Position>(entity).Pos;
                        entityMgr.GetComponent<Position>(entity).Pos = rotationMat * originalPos;
                    }
                }

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(PlayerStateType)) {
                    PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
                    state.isGrounded = true;
                }

                for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(RotationFlagType)) {
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

    void RotationSystem::Shutdown() {
    }
}