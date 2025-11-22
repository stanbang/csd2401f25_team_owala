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
#include "GameManager.h"
#include "OwalaTime.h"

namespace Framework {
	void VFXSystem::Update() {
		GameManager& gm = GameManager::GetInstance();

#ifdef BUILD_EDITOR
		Editor::Editor* editor = gm.GetEditor();
		if (editor) {
			if (!editor->IsGamePlaying() || editor->IsGamePaused()) {
				return;
			}
		}
#endif

		//Get Entity Manager
		EntityManager entityMgr(*entityStorage_);
		EmitParticles(entityMgr);
		UpdateParticles(entityMgr);
	}


	void VFXSystem::EmitParticles(EntityManager& entityMgr) {
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

				Logger::Get().Info(DebugType::VFXSystem) << "Creating particle entity.\n";

				// Create new particle entity
				Entity particleEntity = entityMgr.CreateEntity();

				// Add ParticleComponent
				ParticleComponent particleComp;
				particleComp.effectType = emitter.particleEffectType;
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
				// Generate random direction
				float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f; // Random angle in radians
				float speed = emitter.particleSpeed;
				// Set velocity based on random angle
				velocityComp.Vel.x = speed * cos(angle) * Framework::Time::DeltaTime();
				velocityComp.Vel.y = speed * sin(angle) * Framework::Time::DeltaTime();
				// Add Velocity component to particle
				entityMgr.AddComponent<Velocity>(particleEntity, velocityComp);

				// Add Texture component to the particle based on emitter's provided texture key
				if (!emitter.particleTextureKey.empty()) {
					TextureComponent textureComp;
					textureComp.textureKey = emitter.particleTextureKey;
					textureComp.HalfSize = emitter.particleSize;
					entityMgr.AddComponent<TextureComponent>(particleEntity, textureComp);
				}

				// Add animation component if emitter requires it
				if (emitter.isParticleAnimated) {
					AnimationComponent animComp;
					animComp.isPlaying = true;
					entityMgr.AddComponent<AnimationComponent>(particleEntity, animComp);
				}

				// Add ActiveTag to particle
				entityMgr.AddComponent<ActiveTag>(particleEntity, ActiveTag{});
			}
		}

	}

	void VFXSystem::UpdateParticles(EntityManager& entityMgr) {
		// Loop through all particles
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ParticleComponentType)) {
			if (!entityMgr.HasComponent(entity, ActiveTagType)) continue; // Skip if entity is not active
			if (!entityMgr.HasComponent(entity, ParticleComponentType)) continue; // Skip if entity does not have ParticleComponent
			ParticleComponent& particle = entityMgr.GetComponent<ParticleComponent>(entity);

			// Update particle age
			particle.age += Time::DeltaTime();

			// Check if particle has exceeded its lifetime
			if (particle.age >= particle.lifetime) {
				// Remove particle entity
				entityMgr.DestroyEntity(entity);
			}

			// Effect Type Handling
			switch (particle.effectType) {
			case EffectType::RANDOM:
				// Every 1 second, change direction subtly
				if (static_cast<int>(particle.age) % 1 == 0) {
					if (entityMgr.HasComponent(entity, VelocityType)) {
						Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
						// Apply small random change to velocity
						float angleChange = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f; // Small angle change
						float speed = sqrt(vel.Vel.x * vel.Vel.x + vel.Vel.y * vel.Vel.y);
						float currentAngle = atan2(vel.Vel.y, vel.Vel.x);
						float newAngle = currentAngle + angleChange;
						vel.Vel.x = speed * cos(newAngle);
						vel.Vel.y = speed * sin(newAngle);
						//Logger::Get().Info(DebugType::VFXSystem) << "Particle velocity: (" << vel.Vel.x << ", " << vel.Vel.y << ")\n";
					}
				}
				break;
			case EffectType::NONE:
				break;
			}
		}
	}

	void VFXSystem::Shutdown() {
		// Cleanup if necessary
	}
} // namespace Framework
