/*****************************************************************//**
 * \file   VFXSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "VFXSystem.h"
#include "Logger.h"
#include "Components.h"
#include "EntityManager.h"
#include "OwalaTime.h"

namespace Framework {
	void VFXSystem::Update() {
		//Get Entity Manager
		EntityManager entityMgr(*entityStorage_);
		
	}


	void EmitParticles(EntityManager& entityMgr) {
		// Loop through all particle emitters
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ParticleEmitterComponentType)) {
			if (!entityMgr.HasComponent(entity, ActiveTagType)) continue; // Skip if entity is not active
			if (!entityMgr.HasComponent(entity, ParticleEmitterComponentType)) continue; // Skip if entity does not have ParticleEmitterComponent
			ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(entity);

			// If emission rate is zero or negative, skip emission
			if (emitter.emissionRate <= 0.0f) continue;

			// If not emitting, skip
			if (!emitter.isEmitting) continue;

			// Calculate emission interval
			float emissionInterval = 1.0f / emitter.emissionRate;

			// Update time since last emission
			emitter.timeSinceLastEmission += Time::DeltaTime();

			// Emit particles based on emission rate
			while (emitter.timeSinceLastEmission >= emissionInterval) {

				// Reset timer(but keep any small overflow for accuracy)
				emitter.timeSinceLastEmission -= emissionInterval;

				// Create new particle entity
				Entity particleEntity = entityMgr.CreateEntity();

				// Add ParticleComponent
				ParticleComponent particleComp;
				particleComp.effectType = emitter.isEmitting ? EffectType::RANDOM : EffectType::NONE;
				particleComp.lifetime = emitter.particleLifetime;
				particleComp.age = 0.0f;
				entityMgr.AddComponent<ParticleComponent>(particleEntity, particleComp);

				// Add Position component to particle at emitter's position
				if (entityMgr.HasComponent(entity, PositionType)) {
					Position emitterPos = entityMgr.GetComponent<Position>(entity);
					entityMgr.AddComponent<Position>(particleEntity, emitterPos);
				}

				// Add Velocity component with random direction
				Velocity velocityComp;
				float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f; // Random angle in radians
				float speed = 0.5f; // Set a base speed for particles
				velocityComp.vx = speed * cos(angle);
				velocityComp.vy = speed * sin(angle);
				entityMgr.AddComponent<Velocity>(particleEntity, velocityComp);
				// Additional components like TextureComponent can be added here as needed
			}
		}

	}
} // namespace Framework
