// RotationSystem.cpp
#include "RotationSystem.h"
#include <cmath>
#include <iostream>

namespace Framework {
    RotationSystem::RotationSystem(EntityStorage& ecs, InputSystem& input)
        : ecs_(&ecs), input_(&input) {
    }

    void RotationSystem::Initialize() {
    }

    void RotationSystem::Update(float dt) {
        // Go through all entities that can rotate
        for (auto& [entity, rotationFlag] : ecs_->rotationFlag) {
            if (!rotationFlag.shouldRotate) continue;

            // Check if right arrow key is pressed
            if (input_->KeyPressed(InputSystem::Key::Right)) {
                rotationFlag.targetRotation -= 90.0f; // Rotate clockwise
                rotationFlag.isRotating = true;
                std::cout << "Rotating clockwise 90 degrees" << std::endl;
            }

            // Check if left arrow key is pressed
            if (input_->KeyPressed(InputSystem::Key::Left)) {
                rotationFlag.targetRotation += 90.0f; // Rotate counterclockwise
                rotationFlag.isRotating = true;
                std::cout << "Rotating counterclockwise 90 degrees" << std::endl;
            }

            // If we're currently rotating for smooth transition
            if (rotationFlag.isRotating) {
                float rotationSpeed = 270.0f;

                float rotationThisFrame = rotationSpeed * dt;

                float angleDifference = rotationFlag.targetRotation - rotationFlag.currentRotation;

                if (angleDifference > 180.0f) {
                    angleDifference -= 360.0f;
                }
                if (angleDifference < -180.0f) {
                    angleDifference += 360.0f;
                }

                if (std::abs(angleDifference) > rotationThisFrame) {
                    if (angleDifference > 0) {
                        rotationFlag.currentRotation += rotationThisFrame;
                    }
                    else {
                        rotationFlag.currentRotation -= rotationThisFrame;
                    }
                }
                else {
                    rotationFlag.currentRotation = rotationFlag.targetRotation;
                    rotationFlag.isRotating = false;
                    std::cout << "Finished rotating!" << std::endl;
                }

                // Keep angles between 0 and 360 degrees
                rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
                rotationFlag.targetRotation = std::fmod(rotationFlag.targetRotation + 360.0f, 360.0f);
            }
        }
    }

    void RotationSystem::Shutdown() {
    }
}