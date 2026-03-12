/*********************************************************************//**
 * \file   StageGearRotate.h
 *
 * \primary_author
 *      Jason Lou (100%)
 *
 * \brief  Script for rotating stage gear entities in the level selection screen.
 *         Creates continuous rotation animation for decorative gear elements.
 *
 * \details
 *      This script provides continuous rotation for gear entities in the
 *      level selection screen. It rotates the entity's transform around
 *      the Z-axis at a constant speed, creating a spinning gear effect.
 *      The rotation direction can be configured via the counterClockWise flag.
 *
 *      Key features:
 *        - Continuous rotation animation
 *        - Configurable rotation direction (clockwise/counter-clockwise)
 *        - Automatic rotation speed (hardcoded for consistent visual effect)
 *        - Smooth delta-time based movement
 *
 *      Example Usage:
 *      \code
 *      // Attach to a gear entity in level selection scene
 *      Entity gear = ecs.CreateEntity();
 *      gear.AddComponent<Transform>({100.0f, 200.0f, 0.0f});
 *
 *      auto& script = gear.AddComponent<ScriptComponent>();
 *      script.Bind<StageGearRotate>();
 *
 *      // Configure direction (optional - defaults to counter-clockwise)
 *      auto* rotateScript = script.GetScriptInstance<StageGearRotate>();
 *      rotateScript->counterClockWise = false; // Clockwise rotation
 *      \endcode
 *
 * \note
 *      The rotation speed is currently hardcoded to 1.0 radian per second.
 *      The entity must have a Transform component for rotation to work.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

/**
 * @brief Provides continuous rotation animation for stage gear entities.
 *
 * This script is designed for decorative gear elements in the level selection
 * screen. It continuously rotates the entity's transform around the Z-axis,
 * creating a spinning gear effect. The rotation direction can be configured
 * via the counterClockWise flag.
 *
 * Required components on the same entity:
 * - Transform: For position and rotation data
 */
class StageGearRotate : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the rotation script when the entity is created.
     *
     * Stores the initial rotation value for reference.
     */
    void OnCreate() override;

    /**
     * @brief Updates the rotation each frame.
     *
     * Increments the entity's rotation based on delta time and direction.
     * Rotation speed is fixed at 1.0 radian per second.
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up when the entity is destroyed.
     *
     * Empty implementation as no cleanup is required.
     */
    void OnDestroy() override;

private:
    float prevStageRotation = 0.f; //!< Previous frame's rotation value (unused currently)

public:
    /**
     * @brief Controls the rotation direction.
     *
     * If true, rotates counter-clockwise (increasing angle).
     * If false, rotates clockwise (decreasing angle).
     *
     * Default: true (counter-clockwise)
     */
    bool counterClockWise = true;
};