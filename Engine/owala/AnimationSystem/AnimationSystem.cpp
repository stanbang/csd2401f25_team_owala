/*****************************************************************//**
 * \file   AnimationSystem.cpp
 * \brief
 *
 *
 * \author Ng Jing Wen
 * \date   10 September 2025
 *********************************************************************/
#include "EntityStorage.h"
#include "EntityManager.h"
#include "AnimationSystem.h"
#include <iostream>
#include "WindowsSystem.h" // Include for glfwGetKey
#include <GLFW/glfw3.h>   // Include for key codes

namespace Framework {

    void AnimationSystem::Update(float dt) {
		EntityManager entityMgr(*entityStorage_);
        // Loop through each entity that with AnimationComponent
		for (const Entity& entity : entityMgr.GetAllEntitiesWithComponent(AnimationComponentType)) {
			if (!entityMgr.HasComponent(entity, ActiveTagType)) continue; // Skip if entity is not active
			if (!entityMgr.HasComponent(entity, TextureComponentType)) continue; // Skip if entity does not have TextureComponent
			if (!entityMgr.HasComponent(entity, AnimationComponentType)) continue; // Skip if entity does not have AnimationComponent
			
			AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
			TextureComponent& textureComponent = entityMgr.GetComponent<TextureComponent>(entity);

			// Default facing direction
			bool isFacingRight = true;

			// Handle Player Animation State to Texture Key mapping
			if (entityMgr.HasComponent(entity, PlayerTagType)) {

				// Get PlayerState to determine facing direction
				if (entityMgr.HasComponent(entity, PlayerStateType)) {
					PlayerState& state = entityMgr.GetComponent<PlayerState>(entity);
					isFacingRight = state.isFacingRight;
				}

				switch (animationComponent.currentState) {
				case AnimationState::IDLE:    textureComponent.textureKey = "playerSprite_Idle"; /*std::cout << "Player State : IDLE\n";*/ break; //When idle, use idle animation
				case AnimationState::RUNNING: textureComponent.textureKey = "playerSprite_Run"; /*std::cout << "Player State : RUN\n";*/ break; //When running, use running animation
				case AnimationState::JUMPING: textureComponent.textureKey = "playerSprite_Jump"; /*std::cout << "Player State : JUMP\n";*/ break; //When jumping, use jumping animation
				case AnimationState::FALLING: textureComponent.textureKey = "playerSprite_Jump"; /*std::cout << "Player State : FALL\n";*/ break; //When falling, use jumping animation
				}
			}

			else if (entityMgr.HasComponent(entity, FakePlayerTagType)) {
				switch (animationComponent.currentState) {
				case AnimationState::IDLE:    textureComponent.textureKey = "playerSprite_Idle"; break; //When idle, use idle animation
				case AnimationState::RUNNING: textureComponent.textureKey = "playerSprite_Run"; break; //When running, use running animation
				}
			}

			AnimationInfo& animationInfo = resourceManager_->get<AnimationInfo>(textureComponent.textureKey); // Retrieve the respective AnimationInfo

			// Update animation component properties based on the retrieved AnimationInfo
			animationComponent.frameCount = animationInfo.frameCount;
			animationComponent.frameDuration = animationInfo.frameDuration;
			animationComponent.isLooping = animationInfo.isLooping;
			animationComponent.startFrame = animationInfo.startFrame;

			//	Check if animation is playing
			if (!animationComponent.isPlaying) continue;

			// Update elapsed time
			animationComponent.elapsedTime += dt;

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

			// Calculate UV coordinates based on the current frame
			if (animationComponent.frameCount > 0) {
				float frameUVWidth = 1.0f / (float)animationComponent.frameCount; // Width of each frame in UV space

				// Calculate the standard (right-facing) U coordinates
				float u_min = (float)animationComponent.currentFrame * frameUVWidth;
				float u_max = u_min + frameUVWidth;

				// Apply horizontal flip if not facing right
				if (isFacingRight) {
					animationComponent.minUV.x = u_min;
					animationComponent.maxUV.x = u_max;
				}
				else {
					// Invert horizontally by swapping min and max U
					animationComponent.minUV.x = u_max;
					animationComponent.maxUV.x = u_min;
				}

				// V (Vertical) remains 0.0 to 1.0 for a single-row sprite sheet
				animationComponent.minUV.y = 0.0f;
				animationComponent.maxUV.y = 1.0f;
			}
        }
    }


    void AnimationSystem::Shutdown() {
        
    }

} // namespace Framework
