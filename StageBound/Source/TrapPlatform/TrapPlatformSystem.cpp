/*********************************************************************//**
* \file   TrapPlatformSystem.cpp
*
* \brief
* Implements TrapPlatformSystem. Subscribes to EntityLandedEvent (published
* by PhysicsSystem) and handles all trap-platform-specific landing behaviour:
*   - Resets hasFallen / isGrounded state
*   - Publishes TrapPlatformLandEvent (for animation / other game systems)
*   - Publishes PlaySound3DEvent for the landing sound
*
* Also tags all trap-platform entities with ViewportConstrainedTag so that
* PhysicsSystem::EnforceViewportBounds clamps them without knowing their type.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "pch.h"
#include "TrapPlatformSystem.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "MessageBus.h"
#include "GameMessages.h"
#include "Components.h"

namespace Framework {

    void TrapPlatformSystem::Initialize() {
        MessageBus::Instance().Subscribe<EntityLandedEvent>([](const EntityLandedEvent& ev) {
            EntityManager entityMgr(GameManager::GetInstance().GetECS());

            if (!entityMgr.HasComponent(ev.entity, TrapPlatformComponentType)) return;

            TrapPlatformComponent& trap = entityMgr.GetComponent<TrapPlatformComponent>(ev.entity);

            if (trap.hasFallen && trap.airTime > 0.1f) {
                MessageBus::Instance().Publish(TrapPlatformLandEvent{ ev.position });
                MessageBus::Instance().Publish(PlaySound3DEvent{ "Platform_Land", AudioType::SFX, 1.0f, ev.position });
            }

            trap.hasFallen  = false;
            trap.isGrounded = true;
        });
    }

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
