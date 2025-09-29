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

struct AnimationInfo {
    int frameCount = 0;
    float frameDuration = 0;
    bool isLooping = false;
    int startFrame = 0;
};