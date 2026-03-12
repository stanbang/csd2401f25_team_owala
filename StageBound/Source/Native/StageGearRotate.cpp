/*********************************************************************//**
 * \file   StageGearRotate.cpp
 *
 * \primary_author
 *      Jason Lou (100%)
 *
 * \brief  Implementation of the StageGearRotate script for rotating
 *         stage gear entities in the level selection screen.
 *
 * \details
 *      This file contains the implementation of the gear rotation system.
 *      The script detects when the stage is rotating (e.g., transitioning
 *      between levels) and spins the gear entities accordingly. The rotation
 *      direction is automatically determined based on the gear's name
 *      ("TurningGearL" for counter-clockwise, others for clockwise).
 *
 *      Key features implemented:
 *        - Automatic direction detection from entity name
 *        - Synchronized rotation with stage transitions
 *        - Smooth delta-time based animation
 *        - Degree-to-radian conversion handling
 *        - Angle wrapping to stay within 0-360 degrees
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "StageGearRotate.h"
#include "ScriptRegistry.h"

 // Dependencies
#include "pch.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Components.h"
#include "TransformationSystem.h"
#include "OwalaTime.h"

REGISTER_SCRIPT(StageGearRotate, "StageGearRotate")

// -----------------------------------------------------------------------

/**
 * @brief Initializes the gear rotation script when the entity is created.
 *
 * Determines the rotation direction based on the entity's name:
 * - "TurningGearL" -> counter-clockwise (true)
 * - Any other name -> clockwise (false)
 *
 * This allows level designers to control gear direction simply by naming
 * the entities appropriately in the scene.
 */
    void StageGearRotate::OnCreate() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();

    counterClockWise = entityMgr.GetProperties(self).name == "TurningGearL" ? true : false;
}

// -----------------------------------------------------------------------

/**
 * @brief Updates the gear rotation each frame.
 *
 * This method:
 * 1. Verifies the entity has required components (Texture, Transform)
 * 2. Gets the current stage rotation delta from TransformationSystem
 * 3. Checks if the stage is currently rotating
 * 4. Calculates rotation direction from stage movement
 * 5. Applies rotation to the gear at fixed speed (90 degrees per second)
 * 6. Wraps the angle to stay within 0-360 degrees
 *
 * The gear only rotates when the stage itself is rotating, creating a
 * synchronized visual effect where gears spin during level transitions.
 */
void StageGearRotate::OnUpdate() {
    EntityManager entityMgr(GameManager::GetInstance().GetECS());
    Entity self = GetEntity();
    if (!entityMgr.HasComponent(self, TextureComponentType) || !entityMgr.HasComponent(self, TransformType)) return;

    TransformationSystem& transformSystem = *GameManager::GetInstance().GetSystems().transformation_;

    // Get current stage rotation parameters
    float currentStageRotation = transformSystem.GetGlobalRotation();
    float stageDelta = currentStageRotation - prevStageRotation;
    prevStageRotation = currentStageRotation;

    // Check if stage is currently rotating
    if (!transformSystem.IsGlobalRotating() || stageDelta == 0.f) return;

    // Determine stage rotation direction
    float stageDirection = (stageDelta > 0.f) ? 1.f : -1.f;

    // Rotate the gear if the stage is currently rotating
    float& rotationRad = entityMgr.GetComponent<Transform>(self).rotation.textureAngle;
    float rotationDeg = rotationRad * static_cast<float>((180.f / M_PI));
    float rotationVelocity = 90.f; // Fixed rotation speed: 90 degrees per second

    // Apply direction preference
    if (!counterClockWise) {
        rotationVelocity *= -1.f;
    }

    // Apply Rotation (speed * stage direction * delta time)
    rotationDeg += (rotationVelocity * stageDirection) * Time::DeltaTime();

    // Cleanup - wrap angle to [0, 360) degrees
    rotationDeg = std::fmod(rotationDeg + 360.f, 360.f);
    rotationRad = rotationDeg * static_cast<float>((M_PI / 180.f));
}

// -----------------------------------------------------------------------

/**
 * @brief Cleans up when the entity is destroyed.
 *
 * Empty implementation as no cleanup is required for this script.
 */
void StageGearRotate::OnDestroy() {
    // Logic to be called before Entity is deleted
}