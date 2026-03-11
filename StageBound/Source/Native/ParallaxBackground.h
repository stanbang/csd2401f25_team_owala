/******************************************************************
* \file   ParallaxBackground.h
*
* \primary_author
*       Ng Jing Wen
* \secondary_author
*
*
* \brief	Declares the ParallaxBackground script to create a parallax scrolling effect for
*           the background layers in the scene. Each layer moves at a different speed relative to
*           it's render order.
*
* \copyright Copyright(C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
* ********************************************************************/

#pragma once

#include "ScriptableEntity.h"
#include "Components.h"

using namespace Framework;

class ParallaxBackground : public ScriptableEntity {
protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;

private:
    Vector2D basePosition_{ 0.f, 0.f }; // World position at scene load time
    float parallaxFactor_ = 0.f;        // Fraction of camera offset applied each frame
};
