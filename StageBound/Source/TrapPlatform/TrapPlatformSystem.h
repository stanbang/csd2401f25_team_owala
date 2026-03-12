/*********************************************************************//**
 * \file   TrapPlatformSystem.h
 *
 * \primary_author
 *      Sitara Anil (100%)
 *
 * \brief  Game-layer system responsible for trap-platform landing logic.
 *         Subscribes to the engine's generic EntityLandedEvent and applies
 *         trap-specific state changes.
 *
 * \details
 *      This system manages all trap-platform-specific behaviour in the game.
 *      It listens for EntityLandedEvent (published by PhysicsSystem) and
 *      handles the landing logic for trap platforms:
 *        - Resets hasFallen / isGrounded state
 *        - Publishes TrapPlatformLandEvent (for animation/VFX systems)
 *        - Publishes PlaySound3DEvent for the landing sound
 *
 *      Additionally, it automatically tags all trap-platform entities with
 *      ViewportConstrainedTag, allowing PhysicsSystem to clamp their position
 *      without needing to know the entity type.
 *
 *      Example Usage:
 *      \code
 *      // In game initialization
 *      TrapPlatformSystem trapSystem;
 *      trapSystem.Initialize();
 *
 *      // In game loop
 *      trapSystem.Update();
 *
 *      // Create a trap platform entity (in level loading)
 *      Entity platform = ecs.CreateEntity();
 *      platform.AddComponent<Transform>({x, y, 0});
 *      platform.AddComponent<RigidBodyComponent>();
 *      platform.AddComponent<TrapPlatformComponent>(); // Identifies as trap
 *      \endcode
 *
 * \note
 *      This system depends on PhysicsSystem publishing EntityLandedEvent.
 *      TrapPlatformComponent must be present on entities that should be
 *      treated as trap platforms.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"

namespace Framework {

    /**
     * @brief Manages trap-platform specific behaviour including landing logic
     *        and automatic viewport constraint tagging.
     *
     * This system handles all trap-platform logic that is too specialized for
     * the core PhysicsSystem. It subscribes to generic physics events and
     * translates them into trap-specific behaviour, keeping the engine layer
     * clean and game-agnostic.
     *
     * Key responsibilities:
     * - Respond to EntityLandedEvent for trap platforms
     * - Publish trap-specific events (TrapPlatformLandEvent)
     * - Play 3D landing sounds
     * - Automatically tag trap platforms for viewport constraints
     */
    class TrapPlatformSystem : public ISystem {
    public:
        /**
         * @brief Initializes the system and subscribes to events.
         *
         * Subscribes to EntityLandedEvent from PhysicsSystem to detect
         * when trap platforms land.
         */
        void Initialize() override;

        /**
         * @brief Updates the system each frame.
         *
         * Scans for trap-platform entities and ensures they have the
         * ViewportConstrainedTag for proper viewport clamping.
         */
        void Update() override;

        /**
         * @brief Shuts down the system and cleans up resources.
         *
         * Empty implementation as no cleanup is required.
         */
        void Shutdown() override {}
    };

} // namespace Framework