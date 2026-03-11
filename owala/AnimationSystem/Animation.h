/*****************************************************************//**
 * \file   Animation.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief  Contains the structure definition for AnimationInfo used in ResourceManager.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

using stateToTextureMap = std::unordered_map<int, std::string>; // Map state index (int) to texture key (string)
using animationProfileMap = std::unordered_map<std::string, stateToTextureMap>; // Map entity type (string) to stateToTextureMap

/**
 * This struct represents a segment of a spritesheet used in an animation, 
 * containing the texture key and the number of frames it contributes to the animation.
 */
struct SpriteSheetSegment {
    std::string textureKey;
    int frameCount;
};

/**
 * AnimationInfo struct that holds all necessary information about an animation, 
 * including frame count, duration, looping, and associated spritesheets.
 */
struct AnimationInfo {
    int frameCount = 0;
    float frameDuration = 0;
    bool isLooping = false;
    int startFrame = 0;

    std::vector<SpriteSheetSegment> sheets; // Store relevant spritesheets
	std::unordered_map<int, std::string> frameEvents; // Map specific frames to event names (e.g., frame 5 triggers "PlaySound")
};