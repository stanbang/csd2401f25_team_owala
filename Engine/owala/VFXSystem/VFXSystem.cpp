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

			// Create a temporary origin for calculation. 
			// If emitting in screen space, convert the World Position to Screen Space (0-1).
			Position calculationOrigin = emitterPos;
			if (emitter.isScreenSpace) {
				calculationOrigin.Pos = WorldToScreenSpace(emitterPos.Pos);
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
				particleComp.isScreenSpace = emitter.isScreenSpace;
				entityMgr.AddComponent<ParticleComponent>(particleEntity, particleComp);

				// Calculate emission position based on style
				float rand01 = static_cast<float>(rand()) / RAND_MAX;
				float rand02 = static_cast<float>(rand()) / RAND_MAX;

				// For screen space particles, use normalized coordinates directly (0-1)
				// For world space particles, convert from world to screen then back to world
				Vector2D emissionPos = CalculateEmissionPosition(emitter, calculationOrigin, rand01, rand02, emitter.isScreenSpace);
				Position posComp;
				posComp.Pos = emissionPos;
				entityMgr.AddComponent<Position>(particleEntity, posComp);

				// Calculate emission velocity based on style
				Vector2D emissionVel = CalculateEmissionVelocity(emitter, emissionPos, rand01, rand02, emitter.isScreenSpace);
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
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(ParticleComponentType)) {
			if (!entityMgr.HasComponent(entity, ActiveTagType)) continue;
			if (!entityMgr.HasComponent(entity, ParticleComponentType)) continue;

			ParticleComponent& particle = entityMgr.GetComponent<ParticleComponent>(entity);
			particle.age += Time::DeltaTime();

			if (particle.age >= particle.lifetime) {
				entityMgr.DestroyEntity(entity);
				continue;
			}

			// Screen space particles don't get affected by physics
			if (!particle.isScreenSpace) {
				// Handle physics for world space particles
				switch (particle.effectType) {
				case EffectType::RANDOM: {
					// Apply random jitter to velocity every second
					if (static_cast<int>(particle.age) % 1 == 0) {
						if (entityMgr.HasComponent(entity, VelocityType)) {
							// Apply small random angle change to velocity
							Velocity& vel = entityMgr.GetComponent<Velocity>(entity);
							float angleChange = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.2f;
							float speed = sqrt(vel.Vel.x * vel.Vel.x + vel.Vel.y * vel.Vel.y);
							float currentAngle = atan2(vel.Vel.y, vel.Vel.x);
							float newAngle = currentAngle + angleChange;
							vel.Vel.x = speed * cos(newAngle);
							vel.Vel.y = speed * sin(newAngle);
						}
					}
					break;
				}
				case EffectType::NONE:
				default:
					break;
				}
			}
		}
	}

	void VFXSystem::Shutdown() {
		// Cleanup if necessary
	}

	Vector2D VFXSystem::CalculateEmissionPosition(const ParticleEmitterComponent& emitter,
	const Position& emitterPos, float rand01, float rand02, bool isScreenSpace) {

	Vector2D result;

	switch (emitter.emissionStyle) {
	case EmissionStyle::FROM_POINT: {
		result = emitterPos.Pos;
		break;
	}

	case EmissionStyle::FROM_RADIUS: {
		float angle = rand01 * 2.0f * 3.14159265f;
		float distance = rand02 * emitter.emissionRadius;

		result = Vector2D{
			emitterPos.Pos.x + distance * cos(angle),
			emitterPos.Pos.y + distance * sin(angle)
		};
		break;
	}

	case EmissionStyle::FROM_TOP_VIEWPORT: {
		float x = emitter.viewportMin.x + rand01 * (emitter.viewportMax.x - emitter.viewportMin.x);
		result = Vector2D{ x, emitter.viewportMax.y };
		break;
	}

	case EmissionStyle::FROM_LEFT_VIEWPORT: {
		float y = emitter.viewportMin.y + rand01 * (emitter.viewportMax.y - emitter.viewportMin.y);
		result = Vector2D{ emitter.viewportMin.x, y };
		break;
	}

	case EmissionStyle::FROM_RIGHT_VIEWPORT: {
		float y = emitter.viewportMin.y + rand01 * (emitter.viewportMax.y - emitter.viewportMin.y);
		result = Vector2D{ emitter.viewportMax.x, y };
		break;
	}

	case EmissionStyle::FROM_BOTTOM_VIEWPORT: {
		float x = emitter.viewportMin.x + rand01 * (emitter.viewportMax.x - emitter.viewportMin.x);
		result = Vector2D{ x, emitter.viewportMin.y };
		break;
	}

	case EmissionStyle::FROM_LINE: {
		float t = rand01;
		result = Vector2D{
			emitter.lineStart.x + t * (emitter.lineEnd.x - emitter.lineStart.x),
			emitter.lineStart.y + t * (emitter.lineEnd.y - emitter.lineStart.y)
		};
		break;
	}

	case EmissionStyle::FROM_CIRCLE_EDGE: {
		float angle = rand01 * 2.0f * 3.14159265f;

		result = Vector2D{
			emitterPos.Pos.x + emitter.circleRadius * cos(angle),
			emitterPos.Pos.y + emitter.circleRadius * sin(angle)
		};
		break;
	}

	default:
		result = emitterPos.Pos;
	}

	return result;
}

	Vector2D VFXSystem::CalculateEmissionVelocity(const ParticleEmitterComponent& emitter,
		const Vector2D& emissionPos, float rand01, float rand02, bool isScreenSpace) {

		float angle, speed;

		switch (emitter.emissionStyle) {
		case EmissionStyle::FROM_POINT: {
			angle = rand01 * 2.0f * 3.14159265f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_RADIUS: {
			angle = rand01 * 2.0f * 3.14159265f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_TOP_VIEWPORT: {
			angle = atan2(emitter.emissionDirection.y, emitter.emissionDirection.x);
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_LEFT_VIEWPORT: {
			angle = emitter.emissionDirection.x > 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				(rand01 - 0.5f) * 0.5f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_RIGHT_VIEWPORT: {
			angle = emitter.emissionDirection.x < 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				(rand01 - 0.5f) * 0.5f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_BOTTOM_VIEWPORT: {
			angle = emitter.emissionDirection.y > 0 ?
				atan2(emitter.emissionDirection.y, emitter.emissionDirection.x) :
				3.14159265f / 2.0f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_LINE: {
			Vector2D lineDir = Vector2D{
				emitter.lineEnd.x - emitter.lineStart.x,
				emitter.lineEnd.y - emitter.lineStart.y
			};
			angle = atan2(lineDir.x, -lineDir.y) + (rand01 - 0.5f) * 0.5f;
			speed = emitter.particleSpeed;
			break;
		}

		case EmissionStyle::FROM_CIRCLE_EDGE: {
			float centerAngle = atan2(emissionPos.y - emitter.lineStart.y,
				emissionPos.x - emitter.lineStart.x);
			angle = centerAngle + (rand01 - 0.5f) * 0.3f;
			speed = emitter.particleSpeed;
			break;
		}

		default:
			angle = 0.0f;
			speed = 0.0f;
		}

		// For screen space, don't scale by DeltaTime since we handle it differently
		float dtFactor = isScreenSpace ? 1.0f : Framework::Time::DeltaTime();

		return Vector2D{
			speed * cos(angle) * dtFactor,
			speed * sin(angle) * dtFactor
		};
	}

	Vector2D VFXSystem::WorldToScreenSpace(const Vector2D& worldPos) {
		if (!cameraSystem_) return worldPos;

		Entity activeCamera = cameraSystem_->GetActiveCamera();
		if (activeCamera == 0) return worldPos;

		const Camera& cam = cameraSystem_->GetCamera(activeCamera);

		// Convert world to normalized device coordinates (-1 to 1)
		float screenX = (worldPos.x - cam.position.x) / cam.size.x;
		float screenY = (worldPos.y - cam.position.y) / cam.size.y;

		// Convert NDC to screen space (0 to 1)
		screenX = screenX * 0.5f + 0.5f;
		screenY = screenY * 0.5f + 0.5f;

		return Vector2D{ screenX, screenY };
	}

	Vector2D VFXSystem::ScreenSpaceToWorld(const Vector2D& screenPos) {
		if (!cameraSystem_) return screenPos;

		Entity activeCamera = cameraSystem_->GetActiveCamera();
		if (activeCamera == 0) return screenPos;

		const Camera& cam = cameraSystem_->GetCamera(activeCamera);

		// Convert screen space (0 to 1) to NDC (-1 to 1)
		float ndcX = (screenPos.x - 0.5f) * 2.0f;
		float ndcY = (screenPos.y - 0.5f) * 2.0f;

		// Convert NDC to world
		float worldX = cam.position.x + ndcX * cam.size.x;
		float worldY = cam.position.y + ndcY * cam.size.y;

		return Vector2D{ worldX, worldY };
	}

} // namespace Framework
