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

			// Get emitter position (needed for some emission styles)
			Position emitterPos = { { 0.0f, 0.0f } };
			if (entityMgr.HasComponent(entity, PositionType)) {
				emitterPos = entityMgr.GetComponent<Position>(entity);
			}

			// Emit particles based on emission rate
			while (emitter.timeSinceLastEmission >= emissionInterval) {

				// Reset timer(but keep any small overflow for accuracy)
				emitter.timeSinceLastEmission -= emissionInterval;

				// Create new particle entity
				Entity particleEntity = entityMgr.CreateEntity();

				// Add ParticleComponent
				ParticleComponent particleComp;
				particleComp.effectType = emitter.particleEffectType;
				particleComp.lifetime = emitter.particleLifetime;
				particleComp.age = 0.0f;
				entityMgr.AddComponent<ParticleComponent>(particleEntity, particleComp);

				// Calculate emission position based on style
				float rand01 = static_cast<float>(rand()) / RAND_MAX;
				float rand02 = static_cast<float>(rand()) / RAND_MAX;
				Vector2D emissionPos = CalculateEmissionPosition(emitter, emitterPos, rand01, rand02);

				// Add Position component to particle
				if (entityMgr.HasComponent(entity, PositionType)) {
					entityMgr.AddComponent<Position>(particleEntity, Position{ emissionPos });
				}

				// Calculate emission velocity based on style
				Vector2D emissionVel = CalculateEmissionVelocity(emitter, emissionPos, rand01, rand02);
				Velocity velocityComp;
				velocityComp.Vel = emissionVel;
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

				Logger::Get().Info(DebugType::VFXSystem) << "Created particle at ("
					<< emissionPos.x << ", " << emissionPos.y << ")" << std::endl;
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

	Vector2D VFXSystem::CalculateEmissionPosition(const ParticleEmitterComponent& emitter,
		const Position& emitterPos, float rand01, float rand02) {

		switch (emitter.emissionStyle) {
		case EmissionStyle::FROM_POINT:
			// Simple point emission from entity position
			return emitterPos.Pos;

		case EmissionStyle::FROM_RADIUS: {
			// Random position within a radius
			float angle = rand01 * 2.0f * 3.14159265f;
			float distance = rand02 * emitter.emissionRadius;
			return Vector2D{
				emitterPos.Pos.x + distance * cos(angle),
				emitterPos.Pos.y + distance * sin(angle)
			};
		}

		case EmissionStyle::FROM_TOP_VIEWPORT: {
			// Random x position at top of viewport, y at top
			float x = emitter.viewportMin.x + rand01 *
				(emitter.viewportMax.x - emitter.viewportMin.x);
			return Vector2D{ x, emitter.viewportMax.y };
		}

		case EmissionStyle::FROM_LEFT_VIEWPORT: {
			// Random y position on left edge
			float y = emitter.viewportMin.y + rand01 *
				(emitter.viewportMax.y - emitter.viewportMin.y);
			return Vector2D{ emitter.viewportMin.x, y };
		}

		case EmissionStyle::FROM_RIGHT_VIEWPORT: {
			// Random y position on right edge
			float y = emitter.viewportMin.y + rand01 *
				(emitter.viewportMax.y - emitter.viewportMin.y);
			return Vector2D{ emitter.viewportMax.x, y };
		}

		case EmissionStyle::FROM_BOTTOM_VIEWPORT: {
			// Random x position at bottom of viewport
			float x = emitter.viewportMin.x + rand01 *
				(emitter.viewportMax.x - emitter.viewportMin.x);
			return Vector2D{ x, emitter.viewportMin.y };
		}

		case EmissionStyle::FROM_LINE: {
			// Random position along the line
			float t = rand01;
			return Vector2D{
				emitter.lineStart.x + t * (emitter.lineEnd.x - emitter.lineStart.x),
				emitter.lineStart.y + t * (emitter.lineEnd.y - emitter.lineStart.y)
			};
		}

		case EmissionStyle::FROM_CIRCLE_EDGE: {
			// Position on the edge of a circle around the emitter
			float angle = rand01 * 2.0f * 3.14159265f;
			return Vector2D{
				emitterPos.Pos.x + emitter.circleRadius * cos(angle),
				emitterPos.Pos.y + emitter.circleRadius * sin(angle)
			};
		}

		default:
			return emitterPos.Pos;
		}
	}

	Vector2D VFXSystem::CalculateEmissionVelocity(const ParticleEmitterComponent& emitter,
		const Vector2D& emissionPos, float rand01, float rand02) {

		float angle, speed;

		switch (emitter.emissionStyle) {
		case EmissionStyle::FROM_POINT: {
			// Random direction in all directions
			angle = rand01 * 2.0f * 3.14159265f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_RADIUS: {
			// Random direction (or radial outward)
			angle = rand01 * 2.0f * 3.14159265f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_TOP_VIEWPORT: {
			// Particles fall downward (direction should be set to downward)
			angle = atan2(emitter.emissionDirection.y, emitter.emissionDirection.x);
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_LEFT_VIEWPORT: {
			// Particles move right with slight randomness
			angle = emitter.emissionDirection.x > 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				(rand01 - 0.5f) * 0.5f; // Small random variation
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_RIGHT_VIEWPORT: {
			// Particles move left
			angle = emitter.emissionDirection.x < 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				(rand01 - 0.5f) * 0.5f; // Small random variation
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_BOTTOM_VIEWPORT: {
			// Particles move upward
			angle = emitter.emissionDirection.y > 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				3.14159265f / 2.0f; // Upward
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_LINE: {
			// Direction perpendicular to line or random
			Vector2D lineDir = Vector2D{
				emitter.lineEnd.x - emitter.lineStart.x,
				emitter.lineEnd.y - emitter.lineStart.y
			};
			// Perpendicular direction with randomness
			angle = atan2(lineDir.x, -lineDir.y) + (rand01 - 0.5f) * 0.5f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_CIRCLE_EDGE: {
			// Radial outward or inward from circle edge
			float centerAngle = atan2(emissionPos.y - emitter.lineStart.y,
				emissionPos.x - emitter.lineStart.x);
			angle = centerAngle + (rand01 - 0.5f) * 0.3f; // Small spread
			speed = emitter.particleSpeed;
			break;
		}

		default:
			angle = 0.0f;
			speed = 0.0f;
		}

		return Vector2D{
			speed * cos(angle) * Framework::Time::DeltaTime(),
			speed * sin(angle) * Framework::Time::DeltaTime()
		};
	}

} // namespace Framework
