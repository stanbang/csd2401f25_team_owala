/*********************************************************************
 * \file   VFXSystem.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Contains the declaration of VFXSystem to handle particle effects 
 *			in the game.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "GameMessages.h"
#include <unordered_map>

namespace Framework {
	class WindowsSystem; // Forward declare to get window handle for input
	class VFXSystem : public ISystem {
	public:
		// Constructor
		VFXSystem() = default;
		void Initialize() override;
		void Update() override;
		void Shutdown() override;

	private:
		// Emission methods
		void EmitParticles(EntityManager& entityMgr);
		void UpdateParticles(EntityManager& entityMgr);
		void SpawnSingleParticle(EntityManager& entityMgr, const ParticleEmitterComponent& emitter, const Vector2D& origin);

		// Event handlers for particle burst events
		void OnParticleBurst(const ParticleBurstEvent& event);

		// Trigger system methods
		std::unordered_map<std::string, std::vector<Entity>> triggerToEmitters_;
		void RefreshTriggerRegistrations();
		void RegisterEmitterForTrigger(Entity emitterEntity, const std::string& triggerName);
		void OnWorldRotation(const WorldRotationEvent& event);

		// Generic handler for all trigger events
		void TriggerVFX(const std::string& triggerName);
		void OnTriggerEvent(const std::string& triggerEventName, const Vector2D& position);

		// Helper function to calculate emission position based on style
		Vector2D CalculateEmissionPosition(const ParticleEmitterComponent& emitter,
			const Vector2D& emitterPos, float rand01, float rand02, bool isScreenSpace);
		// Helper function to calculate emission velocity based on style
		Vector2D CalculateEmissionVelocity(const ParticleEmitterComponent& emitter,
			const Vector2D& emissionPos, float rand01, bool isScreenSpace);
		// Convert world position to screen space
		Vector2D WorldToScreenSpace(const Vector2D& worldPos);
		// Convert screen space to world position
		Vector2D ScreenSpaceToWorld(const Vector2D& screenPos);
	};
} // namespace Framework