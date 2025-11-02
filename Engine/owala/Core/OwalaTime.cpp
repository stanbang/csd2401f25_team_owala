#include "OwalaTime.h"

namespace Framework {
	// ---------------------------------------------------------------------------
	// Time Capturing
	// ---------------------------------------------------------------------------
	void Time::Initialize() noexcept {
		now = prev = clock::now();
		dt = 0.0f;
		unscaledDt = 0.0f;
		accumulatedTime = 0.0;
		interpAlpha = 0.0f;
	}

	void Time::BeginFrame() noexcept {
		now = clock::now();
		float frameDuration = std::chrono::duration<float>(now - prev).count();
		if (frameDuration < 0.f) frameDuration = 0.f; // Sanity check for negative time deltas

		unscaledDt = std::min(frameDuration, maxDt); // cap huge delta time
		dt = unscaledDt * timeScale;
		accumulatedTime += static_cast<double>(dt); // Fixed-Step accumulator

		prev = now;
	}

	void Time::EndFrame() noexcept {
		// No-op for now
	}

	// ---------------------------------------------------------------------------
	// Time Capturing
	// ---------------------------------------------------------------------------
	int Time::ConsumeFixedSteps() {
		int steps = 0;
		while (accumulatedTime >= static_cast<double>(fixedDt)) {
			accumulatedTime -= static_cast<double>(fixedDt);
			++steps;
		}

		return steps;
	}
} // namespace Framework