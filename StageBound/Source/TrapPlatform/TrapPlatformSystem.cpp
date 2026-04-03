/*********************************************************************//**
 * \file   TrapPlatformSystem.cpp
 *
 * \primary_author
 *      Sitara Anil (100%)
 *
 * \brief  Implementation of the TrapPlatformSystem that handles trap-platform
 *         landing behaviour and viewport constraints.
 *
 * \details
 *      This system manages all trap-platform-specific logic in the game.
 *      It subscribes to EntityLandedEvent (published by PhysicsSystem) and
 *      handles the landing behaviour for trap platforms:
 *        - Resets hasFallen / isGrounded state
 *        - Publishes TrapPlatformLandEvent (for animation / other game systems)
 *        - Publishes PlaySound3DEvent for the landing sound
 *
 *      Additionally, it automatically tags all trap-platform entities with
 *      ViewportConstrainedTag so that PhysicsSystem::EnforceViewportBounds
 *      can clamp their position without needing to know the entity type.
 *
 * \note
 *      The system only triggers landing sounds and events if the platform
 *      had actually fallen (hasFallen true) and was airborne for >0.1s,
 *      preventing sounds from playing on initial spawn or minor adjustments.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "TrapPlatformSystem.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include "Components.h"

namespace Framework {

    // -----------------------------------------------------------------------

    /**
     * @brief Initializes the TrapPlatformSystem and subscribes to events.
     *
     * Subscribes to EntityLandedEvent to detect when any entity lands.
     * When a trap-platform entity lands, it:
     * 1. Checks if the platform had fallen (hasFallen true) and was airborne >0.1s
     * 2. If yes, publishes a TrapPlatformLandEvent (for animations/VFX) and a
     *    PlaySound3DEvent for the landing sound
     * 3. Resets the platform's state (hasFallen = false, isGrounded = true)
     */
    void TrapPlatformSystem::Initialize() {
        MessageBus::Instance().Subscribe<EntityLandedEvent>([](const EntityLandedEvent& ev) {
            EntityManager entityMgr(GameManager::GetInstance().GetECS());

            // Ignore non-trap-platform entities
            if (!entityMgr.HasComponent(ev.entity, TrapPlatformComponentType)) return;

            TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(ev.entity);

            // Only play effects if the platform actually fell and was airborne
            if (trap.hasFallen && trap.airTime > 0.1f) {
                MessageBus::Instance().Publish(TrapPlatformLandEvent{ ev.position });
                MessageBus::Instance().Publish(PlaySound3DEvent{ "Platform_Land", AudioType::SFX, 0.4f, true, false, ev.position });
            }

            // Reset state for next fall
            trap.hasFallen = false;
            trap.isGrounded = true;
            });
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Updates the system each frame.
     *
     * This method automatically tags every trap-platform entity with a
     * ViewportConstrainedTag. This tag is used by PhysicsSystem's
     * EnforceViewportBounds to clamp the platform's position to the
     * camera viewport without needing to know the entity type.
     *
     * The tagging is done lazily (only if not already tagged) to avoid
     * redundant component additions.
     */
    void TrapPlatformSystem::Update() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Lazily tag every trap-platform entity so EnforceViewportBounds picks it up.
        for (Entity e : entityMgr.GetAllEntitiesWithComponent(true, TrapPlatformComponentType)) {
            if (!entityMgr.HasComponent(e, ViewportConstrainedTagType)) {
                entityMgr.AddComponent(e, ViewportConstrainedTag{});
            }
        }
    }

} // namespace Framework