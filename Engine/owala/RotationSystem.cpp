// RotationSystem.cpp
#include "RotationSystem.h"
#include "Transformation.h"
#include <cmath>
#include <iostream>

namespace Framework {
    RotationSystem::RotationSystem(EntityStorage& ecs, InputSystem& input)
        : ecs_(&ecs), input_(&input), globalRotation(0.0f), targetGlobalRotation(0.0f), isGlobalRotating(false) {
    }

    void RotationSystem::Initialize() {
    }

    void RotationSystem::Update(float dt) {
        if (input_->KeyPressed(InputSystem::Key::Right)) {
            targetGlobalRotation -= 90.0f; // Rotate clockwise
            isGlobalRotating = true;
            //std::cout << "Rotating world clockwise 90 degrees" << std::endl;
        }

        if (input_->KeyPressed(InputSystem::Key::Left)) {
            targetGlobalRotation += 90.0f; // Rotate counterclockwise
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
                //std::cout << "Finished world rotation!" << std::endl;
            }

            globalRotation = std::fmod(globalRotation + 360.0f, 360.0f);
            targetGlobalRotation = std::fmod(targetGlobalRotation + 360.0f, 360.0f);

            if (std::abs(rotationIncrement) > 0.001f) {
                Matrix3x3 rotationMat = Transformation::CreateRotationMatrix(rotationIncrement);

                for (auto& [entity, position] : ecs_->position) {
                    Vector2D originalPos = position.Pos;
                    position.Pos = rotationMat * originalPos;
                }

                for (auto& [entity, rotationFlag] : ecs_->rotationFlag) {
                    rotationFlag.currentRotation += rotationIncrement;
                    rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
                }

            }
        }

        //// Process individual entity rotation
        //for (auto& [entity, rotationFlag] : ecs_->rotationFlag) {
        //    if (!rotationFlag.shouldRotate) continue;

        //    // Handle individual rotation input
        //    if (input_->KeyPressed(InputSystem::Key::Right)) {
        //        rotationFlag.targetRotation -= 90.0f;
        //        rotationFlag.isRotating = true;
        //    }

        //    if (input_->KeyPressed(InputSystem::Key::Left)) {
        //        rotationFlag.targetRotation += 90.0f;
        //        rotationFlag.isRotating = true;
        //    }

        //    // Process the rotation
        //    if (rotationFlag.isRotating) {
        //        float rotationSpeed = 270.0f;
        //        float rotationThisFrame = rotationSpeed * dt;

        //        float angleDifference = rotationFlag.targetRotation - rotationFlag.currentRotation;

        //        // Keep angle difference within -180 to 180 degrees
        //        if (angleDifference > 180.0f) angleDifference -= 360.0f;
        //        if (angleDifference < -180.0f) angleDifference += 360.0f;

        //        if (std::abs(angleDifference) > rotationThisFrame) {
        //            // Rotate part of the way
        //            if (angleDifference > 0) {
        //                rotationFlag.currentRotation += rotationThisFrame;
        //            }
        //            else {
        //                rotationFlag.currentRotation -= rotationThisFrame;
        //            }
        //        }
        //        else {
        //            // Rotate the remaining amount
        //            rotationFlag.currentRotation = rotationFlag.targetRotation;
        //            rotationFlag.isRotating = false;
        //        }

        //        // Keep angles between 0 and 360 degrees
        //        rotationFlag.currentRotation = std::fmod(rotationFlag.currentRotation + 360.0f, 360.0f);
        //        rotationFlag.targetRotation = std::fmod(rotationFlag.targetRotation + 360.0f, 360.0f);
        //    }
        //}
    }

    void RotationSystem::Shutdown() {
    }
}