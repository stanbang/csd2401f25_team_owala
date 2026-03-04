/*****************************************************************//**
 * \file	CutsceneController.h
 *
 * \primary_author
 *      Jason Lou
 * \secondary_author

 *
 * \brief	Contains declarations for the Cutscene Controller.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include "ScriptableEntity.h"

using namespace Framework;

class CutsceneController : public ScriptableEntity {
protected:
	void OnCreate() override;
	void OnUpdate() override;
	void OnDestroy() override;
private:
	/* Data Types */
	struct Slide {
		std::string textureKey;
		float duration;
	};

	/* Variables */
	std::vector<Slide> slides;
	int currentSlide = 0;
	float timer = 0.f;
	bool isEnding = false; // Prevent spam

	/* Variables for Skip handling. */
	float skipHoldTime = 0.f;
	bool wasSkipDown = false;
	bool skipTriggered = false;

	/* Methods */
	void ApplySlide();
	bool NextSlide();
	void FinishCutscene();
	void FitImageToScreen();
	bool IsSkipHeldDown() const;
};
