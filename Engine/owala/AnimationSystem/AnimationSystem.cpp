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
			AnimationComponent& animationComponent = entityMgr.GetComponent<AnimationComponent>(entity);
			TextureComponent& textureComponent = entityMgr.GetComponent<TextureComponent>(entity);
			TextureInfoComponent& textureInfoComponent = entityMgr.GetComponent<TextureInfoComponent>(entity);

			//Check if animation is playing
			if (!animationComponent.isPlaying) continue;

			// Update elapsed time
			animationComponent.elapsedTime += dt;

			//Check if it's time to switch to the next frame
			if (animationComponent.elapsedTime >= textureInfoComponent.frameDuration) {
				animationComponent.elapsedTime -= textureInfoComponent.frameDuration;; // Subtract frame duration(to handle any dt overflow). Eg. elapsedTime = 0.12, 0.12 - 0.1 = 0.02, animation will have 0.08 seconds before next frame change.

				textureInfoComponent.currentFrame++; // Move to the next frame

				//Handle looping at the last frame
				if (textureInfoComponent.currentFrame >= textureInfoComponent.frameCount) {
					if (animationComponent.isLooping) {
						textureInfoComponent.currentFrame = 0; // Loop back to the first frame
					}

					else {
						textureInfoComponent.currentFrame = textureInfoComponent.frameCount - 1; // Stay on the last frame
						textureInfoComponent.isPlaying = false; // Stop the animation
					}
				}
			}

			// Calculate UV coordinates based on the current frame
			if (textureInfoComponent.frameCount > 0) {
				float frameUVWidth = 1.0f / (float)textureInfoComponent.frameCount; // Width of each frame in UV space based on total frame count

				// Calculate U (Horizontal)
				animationComponent.minUV.x = (float)textureInfoComponent.currentFrame * frameUVWidth;
				animationComponent.maxUV.x = animationComponent.minUV.x + frameUVWidth;

				// V (Vertical) remains 0.0 to 1.0 for a single-row sprite sheet
				animationComponent.minUV.y = 0.0f;
				animationComponent.maxUV.y = 1.0f;
			}
        }
    }


    void AnimationSystem::Shutdown() {
        
    }

} // namespace Framework
