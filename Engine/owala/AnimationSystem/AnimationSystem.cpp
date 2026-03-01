/*****************************************************************//**
 * \file   AnimationSystem.cpp
 *
 * \primary_author
 * Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Changes U and V coordinates of Texture components based on specs
 *			from textureInfoComponents to create frame-by-frame animation.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#include "pch.h"
#include "EntityStorage.h"
#include "EntityManager.h"
#include "AnimationSystem.h"
#include "GameManager.h"
#include "OwalaTime.h"
#include "PhysicsSystem.h" // For checking paused state

namespace Framework {

	void AnimationSystem::Update() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;
		PhysicsSystem& physics_ = *GameManager::GetInstance().GetSystems().physics_;
		// If physics is paused, skip animation update
		if (physics_.IsAnimationPaused()) { return; }

		const auto& animationProfiles = resourceManager_.GetAnimationProfiles();
		// Small padding to avoid texture bleeding
		constexpr float UV_PADDING = 0.003f;

		// Loop through each entity that with AnimationComponent
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(true, AnimationComponentType)) {
			if (!entityMgr.HasComponent(entity, LoadedTagType)) continue; // Skip if entity is not active
			if (!entityMgr.HasComponent(entity, TextureComponentType)) continue; // Skip if entity does not have TextureComponent

			AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
			TextureComponent& textureComponent = entityMgr.GetComponent<TextureComponent>(entity);

			if (textureComponent.textureKey.empty()) {
				continue; // Don't load animation for empty textures
			}

			// Store previous texture key to detect changes
			std::string previousTextureKey = textureComponent.textureKey;

			// Default facing direction
			bool isFacingRight = true;
			if (entityMgr.HasComponent(entity, PlayerStateType)) {
				isFacingRight = entityMgr.GetComponent<PlayerState>(entity).isFacingRight;
			}

			if (entityMgr.HasComponent(entity, FSMComponentType)) {
				if (entityMgr.HasComponent(entity, VelocityType)) {
					const Vector2D& vel = entityMgr.GetComponent<Velocity>(entity).Vel;

					// Check current FSM state
					FSMComponent& fsm = entityMgr.GetComponent<FSMComponent>(entity);
					std::string stateName = fsm.fsm.GetCurrentState();

					// //During Attack, textureAngle handles full rotation — skip UV flip
					//if (stateName == "Attack") {
					//	isFacingRight = true; // Lock to default, no UV flip
					//}
					/*else if (entityMgr.HasComponent(entity, VelocityType)) {*/
						
						if (vel.x > 0.1f)       isFacingRight = true;  // flip to face right
						else if (vel.x < -0.1f) isFacingRight = false;   // already faces left
					//}
				}
			}

			if (entityMgr.HasComponent(entity, FSMComponentType)) {
				FSMComponent& fsm = entityMgr.GetComponent<FSMComponent>(entity);
				std::string stateName = fsm.fsm.GetCurrentState();

				// Map FSM States to Animation States for Bird Enemy
				if (stateName == "Idle") {
					animationComponent.currentState = AnimationState::IDLE;
				}
				else if (stateName == "Waking") {
					animationComponent.currentState = AnimationState::WAKING;
				}
				else if (stateName == "SimpleChase" || stateName == "Bounce" || stateName == "Hover") {
					animationComponent.currentState = AnimationState::RUNNING;
				}
				else if (stateName == "Attack") {
					animationComponent.currentState = AnimationState::JUMPING;
				}
				else if (stateName == "Recovery") {
					animationComponent.currentState = AnimationState::RUNNING;
				}
				else if (stateName == "Death") {
					animationComponent.currentState = AnimationState::DEATH;
				}
				// --- ADD WITCH STATE MAPPINGS ---
				else if (stateName == "WitchIdle") {
					animationComponent.currentState = AnimationState::IDLE;
				}
				else if (stateName == "WitchAlerted") {
					animationComponent.currentState = AnimationState::WAKING;
				}
				else if (stateName == "WitchEnraged" || stateName == "WitchCharging") {
					animationComponent.currentState = AnimationState::RUNNING;
				}
				else if (stateName == "WitchTeleport") {
					animationComponent.currentState = AnimationState::JUMPING;
				}
				else if (stateName == "WitchAttack") {
					animationComponent.currentState = AnimationState::HURT;
				}
			}

			// Generic Texture Lookup
			std::string newTextureKey = textureComponent.textureKey; // Default to current

			// Check if this entity uses a profile (e.g "Player")
			if (!animationComponent.profileName.empty()) {
				// Find the profile in the map
				if (animationProfiles.count(animationComponent.profileName)) {
					const auto& stateMap = animationProfiles.at(animationComponent.profileName);
					std::string targetStateString = "";

					// If entity has FSM, the FSM state will determine the animation state
					if (entityMgr.HasComponent(entity, FSMComponentType)) {
						FSMComponent& fsm = entityMgr.GetComponent<FSMComponent>(entity);
						targetStateString = fsm.fsm.GetCurrentState();

						// Convert FSM state to uppercase to match JSON keys (e.g. "Chase" -> "CHASE")
						//std::transform(targetStateString.begin(), targetStateString.end(), targetStateString.begin(), ::toupper);
						// Convert FSM state to uppercase to match JSON keys (e.g. "Chase" -> "CHASE")
						std::transform(targetStateString.begin(), targetStateString.end(), targetStateString.begin(),
							[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
					}

					// Fallback for Player/Non-FSM entities using the legacy AnimationState enum
					else {
						switch (animationComponent.currentState) {
						case AnimationState::IDLE: targetStateString = "IDLE"; break;
						case AnimationState::RUNNING: targetStateString = "RUNNING"; break;
						case AnimationState::JUMPING: targetStateString = "JUMPING"; break;
						case AnimationState::FALLING: targetStateString = "FALLING"; break;
						case AnimationState::DEATH: targetStateString = "DEATH"; break;
						case AnimationState::HURT: targetStateString = "HURT"; break;
						default: targetStateString = "IDLE"; break; // Fallback
						}
					}

					// Look up the texture using the string directly
					if (stateMap.count(targetStateString)) {
						newTextureKey = stateMap.at(targetStateString);
					}
				}
			}

			std::string baseTextureKey = newTextureKey;

			// Load animation info using base texture key
			if (!resourceManager_.has<AnimationInfo>(baseTextureKey)) {
				// Try to load animation info if not already loaded
				try {
					resourceManager_.loadAnimationInfo(baseTextureKey);
				}
				catch (...) {
					Logger::Get().Error(DebugType::ResourceManager)
						<< "Error loading AnimationInfo for texture key: " << baseTextureKey << std::endl;
				}
			}

			if (!resourceManager_.has<AnimationInfo>(baseTextureKey)) {
				// Fallback and treat as static image without animation
				// Reset UVs here to show full image
				animationComponent.isPlaying = false;
				animationComponent.frameCount = 1;
				animationComponent.currentFrame = 0;
				// Force full image display
				animationComponent.minUV = { 0.0f, 0.0f };
				animationComponent.maxUV = { 1.0f, 1.0f };
				continue;
			}
			AnimationInfo& animationInfo = resourceManager_.get<AnimationInfo>(baseTextureKey);

			// Check if the texture key has changed
			bool animationStateChanged = (previousTextureKey != baseTextureKey);

			// Special check: If we are in a multi-sheet animation, previousTextureKey might be "Part_2" 
			// while base is "Part_1". We need to check if previousTextureKey is valid for this animation.
			if (animationStateChanged && !animationInfo.sheets.empty()) {
				for (const auto& sheet : animationInfo.sheets) {
					if (sheet.textureKey == previousTextureKey) {
						animationStateChanged = false; // It's just a different part of the same animation
						break;
					}
				}
			}

			try {
				// Update animation component properties based on the retrieved AnimationInfo
				animationComponent.frameCount = animationInfo.frameCount;
				animationComponent.frameDuration = animationInfo.frameDuration;
				animationComponent.isLooping = animationInfo.isLooping;
				animationComponent.startFrame = animationInfo.startFrame;
			}
			catch (const std::exception& e) {
				Logger::Get().Error(DebugType::ResourceManager)
					<< "Error updating AnimationComponent from AnimationInfo for texture key: "
					<< textureComponent.textureKey << ". Exception: " << e.what() << std::endl;
				continue; // Skip this entity if there's an error
			}

			// If the animation state changed, reset the animation playback
			if (animationStateChanged) {
				try {
					// Update animation component properties based on the retrieved AnimationInfo
					animationComponent.frameCount = animationInfo.frameCount;
					animationComponent.frameDuration = animationInfo.frameDuration;
					animationComponent.isLooping = animationInfo.isLooping; // Only sets default on switch
					animationComponent.startFrame = animationInfo.startFrame;
				}
				catch (const std::exception& e) {
					Logger::Get().Error(DebugType::ResourceManager)
						<< "Error updating AnimationComponent from AnimationInfo for texture key: "
						<< textureComponent.textureKey << ". Exception: " << e.what() << std::endl;
					continue; // Skip this entity if there's an error
				}

				animationComponent.isPlaying = true;
				animationComponent.currentFrame = animationComponent.startFrame;
				animationComponent.elapsedTime = 0.0f;
			}

			//	Check if animation is playing
			if (animationComponent.isPlaying) {

				// Update elapsed time
				animationComponent.elapsedTime += Framework::Time::DeltaTime();

				//	Check if it's time to switch to the next frame
				if (animationComponent.elapsedTime >= animationComponent.frameDuration) {
					animationComponent.elapsedTime -= animationComponent.frameDuration;; // Subtract frame duration(to handle any dt overflow). Eg. elapsedTime = 0.12, 0.12 - 0.1 = 0.02, animation will have 0.08 seconds before next frame change.

					animationComponent.currentFrame++; // Move to the next frame

					//Handle looping at the last frame
					if (animationComponent.currentFrame >= animationComponent.frameCount) {
						if (animationComponent.isLooping) {
							animationComponent.currentFrame = 0; // Loop back to the first frame
						}

						else {
							animationComponent.currentFrame = animationComponent.frameCount - 1; // Stay on the last frame
							animationComponent.isPlaying = false; // Stop the animation
						}
					}
				}
			}

			// Multi spritesheet support
			if (!animationInfo.sheets.empty()) {
				int framesCovered = 0;
				bool foundSheet = false;

				for (const auto& sheet : animationInfo.sheets) {
					// Check if the current global frame falls within this sheet's range
					if (animationComponent.currentFrame < (framesCovered + sheet.frameCount)) {

						// Swap the texture to the specific file for this segment
						textureComponent.textureKey = sheet.textureKey;

						// Calculate "local" frame index (e.g., Global Frame 15 minus 10 previous frames = Local Frame 5)
						int localFrame = animationComponent.currentFrame - framesCovered;

						// Calculate UVs for this specific sheet
						// Note: Using sheet.frameCount, NOT animationComponent.frameCount (which is the total)
						if (sheet.frameCount > 0) {
							float frameUVWidth = 1.0f / (float)sheet.frameCount;
							float u_min = ((float)localFrame * frameUVWidth) + UV_PADDING;
							float u_max = (((float)localFrame + 1.0f) * frameUVWidth) - UV_PADDING;

							if (isFacingRight) {
								animationComponent.minUV.x = u_min;
								animationComponent.maxUV.x = u_max;
							}
							else {
								animationComponent.minUV.x = u_max;
								animationComponent.maxUV.x = u_min;
							}
						}

						// V is standard 0-1 for horizontal strips
						animationComponent.minUV.y = 0.0f;
						animationComponent.maxUV.y = 1.0f;

						foundSheet = true;
						break;
					}
					framesCovered += sheet.frameCount;
				}
			}
			// Fallback for Single-Texture Logic
			else if (animationComponent.frameCount > 0) {
				// Ensure texture key matches the base key
				textureComponent.textureKey = baseTextureKey;

				float frameUVWidth = 1.0f / (float)animationComponent.frameCount;
				float u_min = ((float)animationComponent.currentFrame * frameUVWidth) + UV_PADDING;
				float u_max = (((float)animationComponent.currentFrame + 1.0f) * frameUVWidth) - UV_PADDING;

				if (isFacingRight) {
					animationComponent.minUV.x = u_min;
					animationComponent.maxUV.x = u_max;
				}
				else {
					animationComponent.minUV.x = u_max;
					animationComponent.maxUV.x = u_min;
				}

				animationComponent.minUV.y = 0.0f;
				animationComponent.maxUV.y = 1.0f;
			}
		}
	}


	void AnimationSystem::Shutdown() {

	}

} // namespace Framework