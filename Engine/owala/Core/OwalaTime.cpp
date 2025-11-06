/*****************************************************************//**
* \file   OwalaTime.cpp
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file defines the Time class, which provides an
*			static interface to get time information.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#include "OwalaTime.h"
#include <iostream>

namespace Framework {
	// ---------------------------------------------------------------------------
	// Time Capturing
	// ---------------------------------------------------------------------------
	void Time::Initialize() noexcept {
		now = prev = clock::now();
		dt = 0.0f;
		unscaledDt = 0.0f;
		accumulatedTime = 0.0;
	}

	void Time::BeginFrame() noexcept {
		now = clock::now();
		float frameDuration = std::chrono::duration<float>(now - prev).count();
		if (frameDuration < 0.f) frameDuration = 0.f; // Sanity check for negative time deltas

		unscaledDt = std::min(frameDuration, maxDt); // cap huge delta time
		dt = unscaledDt * timeScale;

		prev = now;
	}

	void Time::EndFrame() noexcept {
		// Empty by design
	}

	// ---------------------------------------------------------------------------
	// Time Capturing
	// ---------------------------------------------------------------------------
	int Time::ConsumeFixedSteps() noexcept {
		if (fixedDt <= 0.0f) return 0;

		int steps = 0;
		accumulatedTime += dt;
		
		while (accumulatedTime >= fixedDt) {
			accumulatedTime -= fixedDt;
			++steps;
		}

		return steps;
	}
} // namespace Framework