#pragma once
#include <chrono>

namespace Framework {
	class Time {
	private:
		// Type Aliases
		using clock = std::chrono::high_resolution_clock;
	private:
		/* Clocks */
		static inline clock::time_point prev;
		static inline clock::time_point now;
		static inline float dt;
		static inline float unscaledDt;

		/* Time Config */
		static inline float fixedDt = 1.0f / 60.0f;
		static inline float maxDt = 0.25f; // Maximum delta time
		static inline float timeScale = 1.0f; // Global time scale factor. Aka Time Dilation.

		/* Accumulated Time */
		static inline double accumulatedTime = 0.0;
		static inline float interpAlpha = 0.0f;
	public:
		/* Time Capturing - Only CoreEngine should be calling these functions */
		static void Initialize() noexcept;
		static void BeginFrame() noexcept;
		static void EndFrame() noexcept;

		/* Getters */
		// The interval in seconds from the last frame to the current frame (Read Only).
		static float DeltaTime() noexcept { return dt; }
		// The interval in seconds with no influence from timescale from the last frame to the current frame (Read Only).
		static float UnscaledDeltaTime() noexcept { return unscaledDt; }

		/* Mutators */
		// The interval in seconds of in-game time at which physics and other fixed frame rate updates are performed.
		static float& FixedDeltaTime() noexcept { return fixedDt; }
		// The maximum value of deltaTime in any given frame. This is a time in seconds that limits the increase of time between two frames.
		static float& MaximumDeltaTime() noexcept { return maxDt; }
		// The scale at which time passes.
		static float& TimeScale() noexcept { return timeScale; }

		/* Time Calculations */
		// Consume and return the number of fixed update steps that can be performed based on the accumulated time. For computing physics.
		static int ConsumeFixedSteps() noexcept;
	};
}