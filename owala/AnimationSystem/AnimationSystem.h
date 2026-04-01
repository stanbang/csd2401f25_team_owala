/*****************************************************************//**
 * \file   AnimationSystem.h
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
#pragma once
#include "ISystem.h"
#include "EntityStorage.h"
#include "ResourceManager.h"

namespace Framework {
	class WindowsSystem; // Forward declare to get window handle for input
	class AnimationSystem : public ISystem {
	public:
		// Constructor
		AnimationSystem() = default;
		void Initialize() override {}
		void Update() override;
		void Shutdown() override;

		// Resets the animation back to its first frame
		static void ResetAnimation(AnimationComponent& animComp);
		
	private:
		// Helper method to reset all animated entities to first frame and calculate UVs
		void ResetAnimationsToFirstFrame();
	};
} // namespace Framework
