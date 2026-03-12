/*********************************************************************//**
 * \file   TimerSystem.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Implementation of the TimerSystem for measuring and managing level
 *      completion times.
 *
 * \details
 *      This file contains the implementation of all TimerSystem methods,
 *      providing core timing functionality for gameplay. The system tracks
 *      elapsed time for the current level and notifies game code via a
 *      callback when the timer stops. It maintains no knowledge of Firebase,
 *      player names, or other game-specific concerns - it simply measures
 *      time and reports completion.
 *
 *      Key features implemented:
 *        - Starting the timer with a specific level name
 *        - Accumulating delta time while running
 *        - Stopping the timer and invoking a completion callback
 *        - Resetting the system state
 *        - Debug logging of timer events
 *
 * \note
 *      This system is designed to be agnostic of game-specific logic.
 *      The onRunFinished callback (set by game code) handles all
 *      post-timer actions like Firebase submission and leaderboard display.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "TimerSystem.h"
#include "Logger.h"

namespace Framework {

    // -----------------------------------------------------------------------

    /**
     * @brief Starts the timer for a specific level.
     *
     * Initializes the timer with zero elapsed time, sets the running flag
     * to true, and stores the provided level name. This method also logs
     * the timer start for debugging purposes.
     *
     * @param levelName The name of the level being timed (used for identification)
     *
     * @note This method does not reset any existing callback - the callback
     *       persists across multiple timer starts/stops.
     */
    void TimerSystem::StartTimer(const std::string& levelName) {
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerSystem] STARTED timer for level: " << levelName << std::endl;
        OutputDebugStringA(("[TimerSystem] STARTED timer for level: " + levelName + "\n").c_str());

        m_elapsed = 0.0f;
        m_running = true;
        m_levelName = levelName;
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Updates the timer with the frame delta time.
     *
     * If the timer is currently running, adds the provided delta time to
     * the elapsed time accumulator. This method should be called every frame
     * from the game's main update loop.
     *
     * @param dt Delta time in seconds since the last frame
     */
    void TimerSystem::Update(float dt) {
        if (m_running)
            m_elapsed += dt;
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Stops the timer and triggers the completion callback.
     *
     * If the timer is running, this method:
     * 1. Sets the running flag to false
     * 2. Logs the final time for debugging
     * 3. Invokes the onRunFinished callback (if set) with the level name
     *    and elapsed time
     *
     * @note The callback is invoked on the same thread that calls Stop().
     * @warning If no callback is registered, the timer stops silently.
     */
    void TimerSystem::Stop() {
        if (!m_running) return;
        m_running = false;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerSystem] STOPPED. Level: " << m_levelName
            << "  Time: " << m_elapsed << "s" << std::endl;

        // Notify game code - no knowledge of Firebase or player names here.
        if (m_onRunFinished)
            m_onRunFinished(m_levelName, m_elapsed);
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Resets the timer to its default state.
     *
     * Clears all timer state:
     * - Stops the timer (sets running to false)
     * - Resets elapsed time to zero
     * - Clears the level name
     *
     * @note This does not affect the registered callback.
     */
    void TimerSystem::Reset() {
        m_running = false;
        m_elapsed = 0.0f;
        m_levelName = "";
    }

} // namespace Framework