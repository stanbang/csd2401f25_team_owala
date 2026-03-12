/*****************************************************************//**
 * \file   VFXSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Implements the VFXSystem to handle particle effects in the game.
 *			Uses event-driven approach and emitter pattern to manage particles.
 *			Contains logic for spawning, updating, and destroying particles as
 *			well as handling different emission styles and effects.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "VFXSystem.h"
#include "Components.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "OwalaTime.h"
#include "ParticleEffectManager.h"

namespace Framework {
	void VFXSystem::Initialize() {
		// Subscribe to the burst event
		MessageBus::Instance().Subscribe<ParticleBurstEvent>(
			[this](const ParticleBurstEvent& e) {
				this->OnParticleBurst(e);
			});
		
		MessageBus::Instance().Subscribe<VFXTriggerEvent>(
			[this](const VFXTriggerEvent& event) {
				Logger::Get().Info(DebugType::VFXSystem)
					<< "Received Generic VFX Trigger: " << event.triggerName << std::endl;

				this->TriggerVFX(event.triggerName, event.overridePosition, event.position);
			});

		// Register all emitters that have triggers
		RefreshTriggerRegistrations();
	}

	void VFXSystem::RefreshTriggerRegistrations() {
		triggerToEmitters_.clear();
		
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		int emittersFound = 0;
		
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, ParticleEmitterComponentType)) {
			ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(entity);
			
			// Skip continuous emitters
			if (emitter.emissionMode == ParticleEmissionMode::CONTINUOUS) {
				continue;
			}
			
			// Check if has trigger event name
			if (!emitter.triggerEventName.empty()) {
				RegisterEmitterForTrigger(entity, emitter.triggerEventName);
				emittersFound++;
				
				Logger::Get().Info(DebugType::VFXSystem)
					<< "Registered emitter (ID: " << entity << ") for trigger: '" << emitter.triggerEventName << "'" << std::endl;
			} else {
				// Log emitters without trigger names
				if (emitter.emissionMode != ParticleEmissionMode::CONTINUOUS) {
					Logger::Get().Error(DebugType::VFXSystem)
						<< "Found non-continuous emitter (ID: " << entity << ") but no trigger event name set!" << std::endl;
				}
			}
		}
		
		Logger::Get().Info(DebugType::VFXSystem)
			<< "Trigger registration refresh complete. Registered " << emittersFound << " emitters" << std::endl;
	}

	void VFXSystem::RegisterEmitterForTrigger(Entity emitterEntity, const std::string& triggerName) {
		triggerToEmitters_[triggerName].push_back(emitterEntity);
	}

	void VFXSystem::OnWorldRotation(const WorldRotationEvent&) {
	EntityManager entityMgr(GameManager::GetInstance().GetECS());
	
	// First, refresh registrations to catch any newly created or reconfigured emitters
	RefreshTriggerRegistrations();
	
	// Find all emitters registered for "WorldRotation" trigger
	auto it = triggerToEmitters_.find("WorldRotationEvent");
	if (it != triggerToEmitters_.end()) {
		Logger::Get().Info(DebugType::VFXSystem)
			<< "WorldRotation event triggered! Found " << it->second.size() << " emitters" << std::endl;
		
		for (Entity emitterEntity : it->second) {
			if (entityMgr.HasComponent(emitterEntity, ParticleEmitterComponentType)) {
				ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(emitterEntity);
				
				Logger::Get().Info(DebugType::VFXSystem)
					<< "Activating burst emitter at position (" << (entityMgr.HasComponent(emitterEntity, TransformType) ? 
					entityMgr.GetComponent<Transform>(emitterEntity).position.x : 0.0f) << ", "
					<< (entityMgr.HasComponent(emitterEntity, TransformType) ? 
					entityMgr.GetComponent<Transform>(emitterEntity).position.y : 0.0f) << ")" << std::endl;
				
				// Get emitter position
				Vector2D emitterPos = { 0.0f, 0.0f };
				if (entityMgr.HasComponent(emitterEntity, TransformType)) {
					emitterPos = entityMgr.GetComponent<Transform>(emitterEntity).position;
				}
				
				Vector2D calculationOrigin = emitterPos;
				if (emitter.isScreenSpace) {
					calculationOrigin = WorldToScreenSpace(emitterPos);
				}
				
				// Emit particles based on mode
				if (emitter.emissionMode == ParticleEmissionMode::BURST) {
					Logger::Get().Info(DebugType::VFXSystem)
						<< "Emitting " << emitter.burstCount << " particles in BURST mode" << std::endl;
					
					for (int i = 0; i < emitter.burstCount; ++i) {
						SpawnSingleParticle(entityMgr, emitter, calculationOrigin);
					}
				} else if (emitter.emissionMode == ParticleEmissionMode::LOOP_BURST) {
					Logger::Get().Info(DebugType::VFXSystem)
						<< "Starting LOOP_BURST mode with interval " << emitter.burstInterval << "s" << std::endl;
					
					emitter.isEmitting = true;
					emitter.timeSinceBurst = 0.0f;
				}
			}
		}
	} else {
		Logger::Get().Error(DebugType::VFXSystem)
			<< "WorldRotation event received, but no emitters registered for 'WorldRotation' trigger!" << std::endl;
		
		// Debug: log all registered triggers
		Logger::Get().Info(DebugType::VFXSystem)
			<< "Available triggers: ";
		for (const auto& pair : triggerToEmitters_) {
			Logger::Get().Info(DebugType::VFXSystem) << "'" << pair.first << "' ";
		}
		Logger::Get().Info(DebugType::VFXSystem) << std::endl;
	}
}

	void VFXSystem::Update() {
#ifdef BUILD_EDITOR
		GameManager& gm = GameManager::GetInstance();
		Editor::Editor* editor = gm.GetEditor();
		if (editor) {
			if (!editor->IsGamePlaying() || editor->IsGamePaused()) {
				return;
			}
		}
#endif

		//Get Entity Manager
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		EmitParticles(entityMgr);
		UpdateParticles(entityMgr);
	}

	// Event Handler for direct burst events
	void VFXSystem::OnParticleBurst(const ParticleBurstEvent& event) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

		ParticleEmitterComponent config;
		config.isEmitting = true;
		config.particleTextureKey = event.textureKey;
		config.particleLifetime = event.lifetime;
		config.particleSpeed = event.speed;
		config.particleSize = event.size;
		config.particleEffectType = event.effect;
		config.effectStrength = event.effectStrength;
		config.isScreenSpace = event.isScreenSpace;
		config.isParticleAnimated = event.isAnimated;

		config.emissionStyle = event.style;
		config.emissionRadius = event.radius;
		config.circleRadius = event.radius;
		config.emissionOffset = event.offset;

		Vector2D origin = event.position;
		if (config.isScreenSpace) {
			origin = WorldToScreenSpace(event.position);
		}

		// Loop 'count' times to spawn the burst
		for (int i = 0; i < event.count; ++i) {
			SpawnSingleParticle(entityMgr, config, origin);
		}
	}

	void VFXSystem::EmitParticles(EntityManager& entityMgr) {
		// Loop through all particle emitters
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, ParticleEmitterComponentType)) {
			if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
			if (!entityMgr.HasComponent(entity, ParticleEmitterComponentType)) continue;

			ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(entity);

			// Handle different emission modes
			switch (emitter.emissionMode) {
			case ParticleEmissionMode::CONTINUOUS: {
				// Original continuous emission logic
				if (emitter.emissionRate <= 0.0f || !emitter.isEmitting) continue;

				float emissionInterval = 1.0f / emitter.emissionRate;
				emitter.timeSinceLastEmission += Time::DeltaTime();

				Vector2D emitterPos = { 0.0f, 0.0f };
				if (entityMgr.HasComponent(entity, TransformType)) {
					emitterPos = entityMgr.GetComponent<Transform>(entity).position;
				}

				Vector2D calculationOrigin = emitterPos;
				if (emitter.isScreenSpace) {
					calculationOrigin = WorldToScreenSpace(emitterPos);
				}

				while (emitter.timeSinceLastEmission >= emissionInterval) {
					emitter.timeSinceLastEmission -= emissionInterval;
					SpawnSingleParticle(entityMgr, emitter, calculationOrigin);
				}
				break;
			}

			case ParticleEmissionMode::BURST: {
				// This mode requires external trigger via MessageBus
				// The emitter waits for an event and emits once
				// Emission happens in separate event handler
				break;
			}

			case ParticleEmissionMode::LOOP_BURST: {
				// Emit multiple bursts at intervals when triggered
				if (!emitter.isEmitting) {
					emitter.timeSinceBurst = 0.0f;
					break;
				}

				emitter.timeSinceBurst += Time::DeltaTime();

				Vector2D emitterPos = { 0.0f, 0.0f };
				if (entityMgr.HasComponent(entity, TransformType)) {
					emitterPos = entityMgr.GetComponent<Transform>(entity).position;
				}

				Vector2D calculationOrigin = emitterPos;
				if (emitter.isScreenSpace) {
					calculationOrigin = WorldToScreenSpace(emitterPos);
				}

				// Check if it's time for the next burst
				if (emitter.timeSinceBurst >= emitter.burstInterval) {
					// Emit a burst
					for (int i = 0; i < emitter.burstCount; ++i) {
						SpawnSingleParticle(entityMgr, emitter, calculationOrigin);
					}

					emitter.timeSinceBurst -= emitter.burstInterval;

					// Auto-disable after bursting
					emitter.isEmitting = false;
				}
				break;
			}

			case ParticleEmissionMode::RANDOM_BURST: {
				if (!emitter.isEmitting) {
					emitter.timeSinceBurst = 0.0f;
					break;
				}

				// Initialize the very first random interval if it's 0
				if (emitter.burstInterval <= 0.0f) {
					float range = std::max(0.001f, emitter.maxBurstInterval - emitter.minBurstInterval);
					emitter.burstInterval = emitter.minBurstInterval + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * range;
				}

				emitter.timeSinceBurst += Time::DeltaTime();

				if (emitter.timeSinceBurst >= emitter.burstInterval) {
					// Calculate dynamic origin
					Vector2D emitterPos = { 0.0f, 0.0f };
					if (entityMgr.HasComponent(entity, TransformType)) {
						emitterPos = entityMgr.GetComponent<Transform>(entity).position;
					}

					Vector2D calculationOrigin = emitterPos;
					if (emitter.isScreenSpace) {
						calculationOrigin = WorldToScreenSpace(emitterPos);
					}

					// Determine random burst count
					int rangeCount = std::max(1, emitter.maxBurstCount - emitter.minBurstCount + 1);
					int currentBurstCount = emitter.minBurstCount + (rand() % rangeCount);

					// Emit the random burst
					for (int i = 0; i < currentBurstCount; ++i) {
						SpawnSingleParticle(entityMgr, emitter, calculationOrigin);
					}

					// Reset timer, preserving any overflow
					emitter.timeSinceBurst -= emitter.burstInterval;

					// Calculate the NEXT random interval
					float rangeTime = std::max(0.001f, emitter.maxBurstInterval - emitter.minBurstInterval);
					emitter.burstInterval = emitter.minBurstInterval + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * rangeTime;
				}
				break;
			}
			}
		}
	}

	void VFXSystem::UpdateParticles(EntityManager& entityMgr) {
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, ParticleComponentType)) {
			if (!entityMgr.HasComponent(entity, LoadedTagType)) continue;
			if (!entityMgr.HasComponent(entity, ParticleComponentType)) continue;

			ParticleComponent& particle = entityMgr.GetComponent<ParticleComponent>(entity);
			particle.age += Time::DeltaTime();

			if (particle.age >= particle.lifetime) {
				entityMgr.DestroyEntity(entity);
				continue;
			}

			if (entityMgr.HasComponent(entity, VelocityType)) {
				Velocity& vel = entityMgr.GetComponent<Velocity>(entity);

				switch (particle.effectType) {
				case EffectType::RANDOM: {
					float speed = sqrtf(vel.Vel.x * vel.Vel.x + vel.Vel.y * vel.Vel.y);
					float jitterStrength = 3.0f * particle.effectStrength * speed * Time::DeltaTime();
					float randomX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * jitterStrength;
					float randomY = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * jitterStrength;
					vel.Vel.x += randomX;
					vel.Vel.y += randomY;
					break;
				}

				case EffectType::DRIFT: {
					float maxAngle = 1.5f * particle.effectStrength * Time::DeltaTime();
					float angle = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f * maxAngle;
					float cosA = cosf(angle), sinA = sinf(angle);
					float newX = vel.Vel.x * cosA - vel.Vel.y * sinA;
					float newY = vel.Vel.x * sinA + vel.Vel.y * cosA;
					vel.Vel.x = newX;
					vel.Vel.y = newY;
					break;
				}

				case EffectType::NONE:
				default:
					break;
				}
			}
		}
	}

	void VFXSystem::SpawnSingleParticle(EntityManager& entityMgr, const ParticleEmitterComponent& emitter, const Vector2D& origin) {
		Entity particleEntity = entityMgr.CreateEntity();

		ParticleComponent particleComp;
		particleComp.effectType = emitter.particleEffectType;
		particleComp.lifetime = emitter.particleLifetime;
		particleComp.age = 0.0f;
		particleComp.isScreenSpace = emitter.isScreenSpace;
		particleComp.effectStrength = emitter.effectStrength;
		entityMgr.AddComponent<ParticleComponent>(particleEntity, particleComp);

		float rand01 = static_cast<float>(rand()) / RAND_MAX;
		float rand02 = static_cast<float>(rand()) / RAND_MAX;

		Vector2D emissionPos = CalculateEmissionPosition(emitter, origin, rand01, rand02, emitter.isScreenSpace);
		entityMgr.AddComponent<Transform>(particleEntity, Transform(emissionPos));

		Vector2D emissionVel = CalculateEmissionVelocity(emitter, emissionPos, rand01, emitter.isScreenSpace);
		Velocity velocityComp;
		velocityComp.Vel = emissionVel;
		entityMgr.AddComponent<Velocity>(particleEntity, velocityComp);

		if (!emitter.particleTextureKey.empty()) {
			TextureComponent textureComp;
			textureComp.textureKey = emitter.particleTextureKey;
			textureComp.HalfSize = emitter.particleSize;
			entityMgr.AddComponent<TextureComponent>(particleEntity, textureComp);
		}

		if (emitter.isParticleAnimated) {
			AnimationComponent animComp;
			animComp.isPlaying = true;
			// Ignore UVs for now to prevent full spritesheet flashing for the first frame.
			animComp.minUV = { 0.0f, 0.0f };
			animComp.maxUV = { 0.0f, 0.0f };
			entityMgr.AddComponent<AnimationComponent>(particleEntity, animComp);
		}

		entityMgr.AddComponent<LoadedTag>(particleEntity, LoadedTag{});
	}

	void VFXSystem::Shutdown() {
		// Cleanup if necessary
	}

	Vector2D VFXSystem::CalculateEmissionPosition(const ParticleEmitterComponent& emitter,
		const Vector2D& emitterPos, float rand01, float rand02, bool isScreenSpace) {

	Vector2D result;

	switch (emitter.emissionStyle) {
	case EmissionStyle::FROM_POINT: {
		result = emitterPos;
		break;
	}

	case EmissionStyle::FROM_RADIUS: {
		float angle = rand01 * 2.0f * 3.14159265f; // Random angle
		float distance = rand02 * emitter.emissionRadius; // Random distance within radius

		result = Vector2D{
			emitterPos.x + distance * cos(angle),
			emitterPos.y + distance * sin(angle)
		};
		break;
	}

	case EmissionStyle::FROM_TOP_VIEWPORT: {
		if (isScreenSpace) {
			float x = rand01;
			result = Vector2D{ x, 1.0f };
		}
		else {
			// Dynamically fetch actual camera bounds
			CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
			if (camSys && camSys->GetActiveCamera() != 0) {
				float left, right, bottom, top;
				camSys->GetViewBounds(camSys->GetActiveCamera(), left, right, bottom, top);

				float x = left + rand01 * (right - left);
				result = Vector2D{ x, top };
			}
			else {
				// Fallback if no camera exists
				float x = emitter.viewportMin.x + rand01 * (emitter.viewportMax.x - emitter.viewportMin.x);
				result = Vector2D{ x, emitter.viewportMax.y };
			}
		}
		break;
	}
	case EmissionStyle::FROM_LEFT_VIEWPORT: {
		if (isScreenSpace) {
			// Screen space: spawn at left edge (x=0)
			float y = rand01;
			result = Vector2D{ 0.0f, y };
		}
		else {
			// Dynamically fetch actual camera bounds
			CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
			if (camSys && camSys->GetActiveCamera() != 0) {
				float left, right, bottom, top;
				camSys->GetViewBounds(camSys->GetActiveCamera(), left, right, bottom, top);

				float y = bottom + rand01 * (top - bottom);
				result = Vector2D{ left, y };
			}
			else {
				// Fallback
				float y = emitter.viewportMin.y + rand01 * (emitter.viewportMax.y - emitter.viewportMin.y);
				result = Vector2D{ emitter.viewportMin.x, y };
			}
		}
		break;
	}

	case EmissionStyle::FROM_RIGHT_VIEWPORT: {
		if (isScreenSpace) {
			// Screen space: spawn at right edge (x=1)
			float y = rand01;
			result = Vector2D{ 1.0f, y };
		}
		else {
			// Dynamically fetch actual camera bounds
			CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
			if (camSys && camSys->GetActiveCamera() != 0) {
				float left, right, bottom, top;
				camSys->GetViewBounds(camSys->GetActiveCamera(), left, right, bottom, top);

				float y = bottom + rand01 * (top - bottom);
				result = Vector2D{ right, y };
			}
			else {
				// Fallback
				float y = emitter.viewportMin.y + rand01 * (emitter.viewportMax.y - emitter.viewportMin.y);
				result = Vector2D{ emitter.viewportMax.x, y };
			}
		}
		break;
	}

	case EmissionStyle::FROM_BOTTOM_VIEWPORT: {
		if (isScreenSpace) {
			// Screen space: spawn at bottom (y=0)
			float x = rand01;
			result = Vector2D{ x, 0.0f };
		}
		else {
			// Dynamically fetch actual camera bounds
			CameraSystem* camSys = GameManager::GetInstance().GetSystems().camera_;
			if (camSys && camSys->GetActiveCamera() != 0) {
				float left, right, bottom, top;
				camSys->GetViewBounds(camSys->GetActiveCamera(), left, right, bottom, top);

				float x = left + rand01 * (right - left);
				result = Vector2D{ x, bottom };
			}
			else {
				// Fallback
				float x = emitter.viewportMin.x + rand01 * (emitter.viewportMax.x - emitter.viewportMin.x);
				result = Vector2D{ x, emitter.viewportMin.y };
			}
		}
		break;
	}

	// Emit along a line defined by lineStart and lineEnd
	case EmissionStyle::FROM_LINE: {
		float t = rand01;
		result = Vector2D{
			emitter.lineStart.x + t * (emitter.lineEnd.x - emitter.lineStart.x),
			emitter.lineStart.y + t * (emitter.lineEnd.y - emitter.lineStart.y)
		};
		break;
	}

	// Imagine a circle centered at emitterPos with radius circleRadius
	case EmissionStyle::FROM_CIRCLE_EDGE: {
		float angle = rand01 * 2.0f * 3.14159265f; // Random angle

		result = Vector2D{
			emitterPos.x + emitter.circleRadius * cos(angle),
			emitterPos.y + emitter.circleRadius * sin(angle)
		};
		break;
	}

	default:
		result = emitterPos;
	}

	// Apply emission offset after calculating base position
	result.x += emitter.emissionOffset.x;
	result.y += emitter.emissionOffset.y;

	return result;
}

// Calculate initial velocity based on emission style
	Vector2D VFXSystem::CalculateEmissionVelocity(const ParticleEmitterComponent& emitter,
		const Vector2D& emissionPos, float rand01, bool isScreenSpace) {

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

		return Vector2D{
			speed * cos(angle),
			speed * sin(angle)
		};
	}

	Vector2D VFXSystem::WorldToScreenSpace(const Vector2D& worldPos) {
		CameraSystem* cameraSystem_ = GameManager::GetInstance().GetSystems().camera_;
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
		CameraSystem* cameraSystem_ = GameManager::GetInstance().GetSystems().camera_;
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

	// Generic handler for all trigger events
	void VFXSystem::TriggerVFX(const std::string& triggerName, bool hasPositionOverride, Vector2D overridePos) {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		RefreshTriggerRegistrations(); // Ensure registrations are up-to-date

		Logger::Get().Info(DebugType::VFXSystem)
			<< "TriggerVFX called for trigger: '" << triggerName << "'" << std::endl;

		// Handle emitters (Entities existing in the level, e.g. Dust)
		auto it = triggerToEmitters_.find(triggerName);
		if (it != triggerToEmitters_.end()) {
			Logger::Get().Info(DebugType::VFXSystem)
				<< "Found " << it->second.size() << " emitters for trigger: '" << triggerName << "'" << std::endl;
			for (Entity emitterEntity : it->second) {
				if (!entityMgr.HasComponent(emitterEntity, ParticleEmitterComponentType)) continue;
				ParticleEmitterComponent& emitter = entityMgr.GetComponent<ParticleEmitterComponent>(emitterEntity);

				// Check if this trigger has a JSON definition
				std::vector<std::string> jsonEffects = ParticleEffectManager::GetInstance().GetEffectsByTrigger(triggerName);

				if (!jsonEffects.empty()) {
					// If JSON exists, use its data instead of the component's raw data
					ParticleBurstEvent preset;
					// Just use the first mapped effect for this trigger
					if (ParticleEffectManager::GetInstance().GetEffectPreset(jsonEffects[0], preset)) {

						// Create a temporary emitter config using JSON data
						ParticleEmitterComponent overrideEmitter = emitter;
						overrideEmitter.particleTextureKey = preset.textureKey;
						overrideEmitter.particleLifetime = preset.lifetime;
						overrideEmitter.particleSpeed = preset.speed;
						overrideEmitter.particleSize = preset.size;
						overrideEmitter.particleEffectType = preset.effect;
						overrideEmitter.effectStrength = preset.effectStrength;
						overrideEmitter.isScreenSpace = preset.isScreenSpace;
						overrideEmitter.isParticleAnimated = preset.isAnimated;
						overrideEmitter.emissionStyle = preset.style;
						overrideEmitter.emissionRadius = preset.radius;
						overrideEmitter.emissionOffset = preset.offset;
						overrideEmitter.burstCount = preset.count;

						// Calculate Position based on the entity
						Vector2D emitterPos = { 0.0f, 0.0f };
						if (entityMgr.HasComponent(emitterEntity, TransformType)) {
							emitterPos = entityMgr.GetComponent<Transform>(emitterEntity).position;
						}
						Vector2D calculationOrigin = overrideEmitter.isScreenSpace ? WorldToScreenSpace(emitterPos) : emitterPos;

						// Spawn using JSON settings and Entity Position
						for (int i = 0; i < overrideEmitter.burstCount; ++i) {
							SpawnSingleParticle(entityMgr, overrideEmitter, calculationOrigin);
						}
						continue; // Skip the default behavior below
					}
				}

				// Fallback: If no JSON found, use the Component's raw inspector values
				Vector2D emitterPos = { 0.0f, 0.0f };
				// Get emitter position
				if (entityMgr.HasComponent(emitterEntity, TransformType)) {
					emitterPos = entityMgr.GetComponent<Transform>(emitterEntity).position;
				}

				// Calculate origin
				Vector2D calculationOrigin = emitter.isScreenSpace ? WorldToScreenSpace(emitterPos) : emitterPos;

				// Emit particles based on mode
				if (emitter.emissionMode == ParticleEmissionMode::BURST) {
					for (int i = 0; i < emitter.burstCount; ++i) {
						SpawnSingleParticle(entityMgr, emitter, calculationOrigin);
					}
				}
				else if (emitter.emissionMode == ParticleEmissionMode::LOOP_BURST) {
					emitter.isEmitting = true;
					emitter.timeSinceBurst = 0.0f;
				}
			}
		}

		// Handle presets (defined in JSON, e.g. WorldRotationEvent)
		std::vector<std::string> effects = ParticleEffectManager::GetInstance().GetEffectsByTrigger(triggerName);
		for (const std::string& effectName : effects) {
			ParticleBurstEvent burst;
			if (ParticleEffectManager::GetInstance().GetEffectPreset(effectName, burst)) {
				if (burst.isGlobal == false) {
					continue; // Do NOT spawn globally. Wait for a component to pick it up.
				}

				Logger::Get().Info(DebugType::VFXSystem)
					<< "Triggering preset effect: '" << effectName << "' for trigger: '" << triggerName << "'" << std::endl;
				
				// Apply position override if specified
				if (hasPositionOverride) {
					burst.position = overridePos;
				}

				this->OnParticleBurst(burst);
			}
		}
	}
} // namespace Framework
