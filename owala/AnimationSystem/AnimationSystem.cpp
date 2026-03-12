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

	/**
	 * Update the animation system for all entities with AnimationComponent. This involves:
	 * Checking if the entity is active and has a TextureComponent, determining the correct 
	 * animation frame based on the entity's state (e.g. FSM state or player state), and 
	 * updating the UV coordinates of the TextureComponent to display the correct frame of 
	 * the animation.
	 */
	void AnimationSystem::Update() {
		EntityManager entityMgr(GameManager::GetInstance().GetECS());
		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;
		PhysicsSystem& physics_ = *GameManager::GetInstance().GetSystems().physics_;

		// If physics is paused, skip animation update
		if (physics_.IsAnimationPaused()) { return; }

		#ifdef BUILD_EDITOR
		// In editor mode, only update animations when the game is actively playing
		if (auto* editor = GameManager::GetInstance().GetEditor()) {
			if (!editor->IsGamePlaying()) { return; }
		}
		#endif

		// Get the animation profiles mapping from the ResourceManager
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

			// For player entities, we can determine facing direction from PlayerState or Velocity
			if (entityMgr.HasComponent(entity, PlayerStateType)) {
				isFacingRight = entityMgr.GetComponent<PlayerState>(entity).isFacingRight;
			}

			// For non-player entities, we can infer facing direction from Velocity (if available)
			if (entityMgr.HasComponent(entity, VelocityType)) {
				const Vector2D& vel = entityMgr.GetComponent<Velocity>(entity).Vel;
				if (vel.x > 0.1f)       isFacingRight = true; // Moving right
				else if (vel.x < -0.1f) isFacingRight = false; // Moving left
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
						targetStateString = fsm.fsm.GetCurrentState(); // Get current FSM state as string

						// Convert FSM state to uppercase to match JSON keys (e.g. "Chase" -> "CHASE")
						// std::transform(targetStateString.begin(), targetStateString.end(), targetStateString.begin(), ::toupper);
						// Convert FSM state to uppercase to match JSON keys (e.g. "Chase" -> "CHASE")
						std::transform(targetStateString.begin(), targetStateString.end(), targetStateString.begin(),
							[](unsigned char c) { return static_cast<char>(std::toupper(c)); });
					}

					// Non-FSM entities (e.g. player): read state string set directly by game code
					else {
						targetStateString = animationComponent.currentState;
					}

					// Look up the texture using the string directly
					if (stateMap.count(targetStateString)) {
						newTextureKey = stateMap.at(targetStateString);
					}
				}
			}

			// When the texture key changes, we need to check if the new texture has animation info and update the animation component accordingly
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

			// If there's still no animation info, this texture is not animated. We can skip the rest of the loop and treat it as a static image.
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

			// If we have animation info, we can update the animation component properties accordingly
			AnimationInfo& animationInfo = resourceManager_.get<AnimationInfo>(baseTextureKey);

			// Check if the texture key has changed
			bool animationStateChanged = (previousTextureKey != baseTextureKey);

			// Special check: If we are in a multi-sheet animation, previousTextureKey might be "Part_2" 
			// while base is "Part_1". We need to check if previousTextureKey is valid for this animation.
			if (animationStateChanged && !animationInfo.sheets.empty()) {
				for (const auto& sheet : animationInfo.sheets) { // Iterate through all sheets in this animation
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
				// Reset delay so it re-applies after each state change
				animationComponent.delayRemaining = -1.0f;
			}

			// Initialise start delay on first encounter (delayRemaining == -1) or after a state change
			if (animationComponent.delayRemaining < 0.0f) {
				float delay = animationComponent.startDelay;
				// If startDelayRange is set, add a random amount in [0, startDelayRange] to the delay
				if (animationComponent.startDelayRange > 0.0f) {
					float random01 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
					delay += random01 * animationComponent.startDelayRange;
				}

				// Set the delayRemaining to the calculated delay
				animationComponent.delayRemaining = delay;

				// If there's a delay, we should hold playback until the delay expires
				if (delay > 0.0f) {
					animationComponent.isPlaying = false; // Hold playback until delay expires
				}
			}

			// Count down the start delay before allowing frames to advance
			if (animationComponent.delayRemaining > 0.0f) {
				animationComponent.delayRemaining -= Framework::Time::DeltaTime();
				if (animationComponent.delayRemaining <= 0.0f) {
					animationComponent.delayRemaining = 0.0f;
					animationComponent.isPlaying = true; // Delay has elapsed, begin playback
				}
			}

			//	Check if animation is playing
			if (animationComponent.isPlaying) {

				// Update elapsed time
				animationComponent.elapsedTime += Framework::Time::DeltaTime();

				//	Check if it's time to switch to the next frame
				if (animationComponent.elapsedTime >= animationComponent.frameDuration) {
					animationComponent.elapsedTime -= animationComponent.frameDuration;; // Subtract frame duration(to handle any dt overflow). Eg. elapsedTime = 0.12, 0.12 - 0.1 = 0.02, animation will have 0.08 seconds before next frame change.
					animationComponent.currentFrame++; // Move to the next frame

					if (animationInfo.frameEvents.count(animationComponent.currentFrame)) {
						// Get whatever string was set in the Editor/JSON for this frame
						const std::string& eventString = animationInfo.frameEvents.at(animationComponent.currentFrame);

						// Broadcast the generic event
						MessageBus::Instance().Publish(AnimationEvent{ entity, eventString });
					}

					//Handle looping at the last frame
					if (animationComponent.currentFrame >= animationComponent.frameCount) {
						if (animationComponent.isLooping) {
							animationComponent.currentFrame = 0; // Loop back to the first frame
							animationComponent.delayRemaining = -1.0f; // Re-trigger delay between loops
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

				float frameUVWidth = 1.0f / (float)animationComponent.frameCount; // Calculate UVs based on the current frame index
				float u_min = ((float)animationComponent.currentFrame * frameUVWidth) + UV_PADDING; // Add small padding to avoid texture bleeding
				float u_max = (((float)animationComponent.currentFrame + 1.0f) * frameUVWidth) - UV_PADDING; // Subtract padding on the max UV as well

				// Handle horizontal flipping by swapping UVs
				if (isFacingRight) {
					animationComponent.minUV.x = u_min;
					animationComponent.maxUV.x = u_max;
				}
				// If not facing right, we flip the UVs to mirror the image
				else {
					animationComponent.minUV.x = u_max;
					animationComponent.maxUV.x = u_min;
				}

				// UV.y is standard 0-1 for horizontal strips
				animationComponent.minUV.y = 0.0f;
				animationComponent.maxUV.y = 1.0f;
			}
		}
	}


	void AnimationSystem::Shutdown() {

	}

	void AnimationSystem::ResetAnimation(AnimationComponent& animComp) {
		animComp.currentFrame = animComp.startFrame;
		animComp.elapsedTime = 0.0f;
		animComp.isPlaying = true;
		animComp.delayRemaining = -1.0f;
	}

} // namespace Framework