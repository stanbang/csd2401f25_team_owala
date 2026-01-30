/*****************************************************************//**
* \file   OwalaTime.h
*
* \primary_author
*       Jason Lou
* \secondary_author
*
*
* \brief    This file declares the Time class, which provides an
*			static interface to get time information.
*
* \copyright Copyright (C) 2025 DigiPen Institute of Technology.
* Reproduction or disclosure of this file or its contents without the prior
* written consent of DigiPen Institute of Technology is prohibited.
*********************************************************************/
#pragma once
#include <chrono>

namespace Framework {
	class Time {
	private:
		// Type Aliases
		using clock = std::chrono::high_resolution_clock;
	private:
		/* Clocks */
		static inline clock::time_point prev; // Internal time recorded at the previous frame.
		static inline clock::time_point now; // Internal time recorded at the current frame.
		static inline float dt; // Delta time, or time elapsed since the previous frame. With respect to timeScale.
		static inline float unscaledDt; // Delta time, unaffected by the timeScale.

		/* Time Config */
		static inline float fixedDt = 1.0f / 60.0f; // Fixed DeltaTime
		static inline float maxDt = 0.25f; // Maximum delta time
		static inline float timeScale = 1.0f; // Global time scale factor. Aka Time Dilation.

		/* Accumulated Time */
		static inline double accumulatedTime = 0.0; // Internal Time
		static inline double totalAppTime = 0.0;
	public:
		/* Time Capturing - Only CoreEngine should be calling these functions */
		// Let Engine call this
		static void Initialize() noexcept;
		// Let Engine call this
		static void BeginFrame() noexcept;
		// Let Engine call this
		static void EndFrame() noexcept;

		/* Getters */
		// The interval in seconds from the last frame to the current frame (Read Only).
		static float DeltaTime() noexcept { return dt; }
		// The interval in seconds with no influence from timescale from the last frame to the current frame (Read Only).
		static float UnscaledDeltaTime() noexcept { return unscaledDt; }
		// Total runtime
		static float TotalRuntime() noexcept { return static_cast<float>(totalAppTime); }

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