/*****************************************************************//**
 * \file   AnimationSystem.cpp
 *
 * \primary_author
 *      Ng Jing Wen
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
#include "EntityStorage.h"
#include "EntityManager.h"
#include "AnimationSystem.h"
#include <iostream>
#include "GameManager.h"
#include "OwalaTime.h"

namespace Framework {

    void AnimationSystem::Update() {

		ResourceManager& resourceManager_ = *GameManager::GetInstance().GetSystems().resourceManager_;
		EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Loop through each entity that with AnimationComponent
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(AnimationComponentType)) {
			if (!entityMgr.HasComponent(entity, ActiveTagType)) continue; // Skip if entity is not active
			if (!entityMgr.HasComponent(entity, TextureComponentType)) continue; // Skip if entity does not have TextureComponent
			if (!entityMgr.HasComponent(entity, AnimationComponentType)) continue; // Skip if entity does not have AnimationComponent
			
			AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
			TextureComponent& textureComponent = entityMgr.GetComponent<TextureComponent>(entity);

			if (textureComponent.textureKey.empty()) {
				continue; // Don't try to load animation for empty texture
			}

			// Default facing direction
			bool isFacingRight = true;
			// The last texture key used
			std::string previousTextureKey = textureComponent.textureKey;
			std::string baseTextureKey = textureComponent.textureKey;

			// Handle Player Animation State to Texture Key mapping
			if (entityMgr.HasComponent(entity, PlayerTagType)) {

				// Get PlayerState to determine facing direction
				if (entityMgr.HasComponent(entity, PlayerStateType)) {
					PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
					isFacingRight = state.isFacingRight;
				}

				// Map AnimationState to Texture Key
				switch (animationComponent.currentState) {
				case AnimationState::IDLE:    baseTextureKey = "SB_Eli_Idle_Spritesheet_1.png"; /*std::cout << "Player State : IDLE\n";*/ break; //When idle, use idle animation
				case AnimationState::RUNNING: baseTextureKey = "Eli_Walk_Spritesheet.png"; /*std::cout << "Player State : RUN\n";*/ break; //When running, use running animation
				case AnimationState::JUMPING: baseTextureKey = "Eli_Jump_Spritesheet.png"; /*std::cout << "Player State : JUMP\n";*/ break; //When jumping, use jumping animation
				case AnimationState::FALLING: baseTextureKey = "Eli_Falling_Spritesheet.png"; /*std::cout << "Player State : FALL\n";*/ break; //When falling, use jumping animation
				case AnimationState::DEATH:   baseTextureKey = "SB_Eli_Death_Spritesheet_1.png";/*std::cout << "Player State : DEATH \n";*/ break; // When dying, use death animation
				}

			}

			else if (entityMgr.HasComponent(entity, FakePlayerTagType)) {
				switch (animationComponent.currentState) {
				case AnimationState::IDLE:    baseTextureKey = "SB_Eli_Idle_Spritesheet_1.png"; break; //When idle, use idle animation
				case AnimationState::RUNNING: baseTextureKey = "Eli_Walk_Spritesheet.png"; break; //When running, use running animation
				}
			}

			// Handle bird animations
			else if (entityMgr.HasComponent(entity, BirdAIComponentType)) {
				// Get BirdAIComponent to determine state
				BirdAIComponent& birdAI = entityMgr.GetComponent<BirdAIComponent>(entity);

				isFacingRight = birdAI.isFacingRight; // use bird facing direction

				switch (birdAI.state) {
				case BirdState::IDLE: baseTextureKey = "SB_BirdEnemy_SleepIdle_Spritesheet.png"; break;
				case BirdState::WAKING: baseTextureKey = "SB_BirdEnemy_SleepIdle_Spritesheet.png"; break;
				case BirdState::CHASE: baseTextureKey = "SB_BirdEnemy_Fly_Spritesheet_1.png"; break;
				case BirdState::HOVER: baseTextureKey = "SB_BirdEnemy_Fly_Spritesheet_1.png"; break;
				case BirdState::ATTACK: baseTextureKey = "SB_BirdEnemy_DiveAttack_Spritesheet.png"; break;
				}
			}

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
				animationComponent.isPlaying = true;
				animationComponent.currentFrame = animationComponent.startFrame;
				animationComponent.elapsedTime = 0.0f;
			}

			//	Check if animation is playing
			if (!animationComponent.isPlaying) continue;

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
							float u_min = (float)localFrame * frameUVWidth;
							float u_max = u_min + frameUVWidth;

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
				float u_min = (float)animationComponent.currentFrame * frameUVWidth;
				float u_max = u_min + frameUVWidth;

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
