/*********************************************************************//**
 * \file   TimerSystem.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the TimerSystem singleton for measuring and managing
 *      level completion times.
 *
 * \details
 *      The TimerSystem provides core timing functionality for gameplay,
 *      tracking elapsed time for the current level run. It is designed as an
 *      engine-layer component with no game-specific dependencies. When a run
 *      ends, it fires a callback that game code can use to handle the result
 *      (submit to Firebase, save locally, display UI, etc.).
 *
 *      Key features:
 *        - Start/stop/reset timing for named levels
 *        - Per-frame update with delta time accumulation
 *        - Query functions for current state (running, elapsed time, level)
 *        - Configurable callback for run completion events
 *
 *      Example Usage:
 *      \code
 *      // Initialize at startup - set up completion handler
 *      TimerSystem::Get().SetOnRunFinished(
 *          [](const std::string& level, float time) {
 *              // Game-specific logic here
 *              if (GameStateFlags::shouldRecordTime) {
 *                  FirebaseManager::Get().SubmitRun(
 *                      level,
 *                      PlayerNameManager::Get().GetName(),
 *                      time
 *                  );
 *              }
 *              // Show leaderboard, etc.
 *          }
 *      );
 *
 *      // In level start trigger
 *      TimerSystem::Get().StartTimer("Level1");
 *
 *      // In game loop update
 *      TimerSystem::Get().Update(deltaTime);
 *
 *      // In level end trigger
 *      TimerSystem::Get().Stop();  // Invokes callback
 *
 *      // Reset when leaving gameplay
 *      TimerSystem::Get().Reset();
 *      \endcode
 *
 * \note
 *      This system is agnostic of Firebase, player names, and other game
 *      concerns. All game-specific logic should be implemented in the
 *      callback set via SetOnRunFinished().
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>
#include <functional>

namespace Framework {

    // ------------------------------------------------------------------
    // Timer System (Singleton)
    // ------------------------------------------------------------------
    /**
     * @brief Manages level run timing with no game-specific dependencies.
     *
     * This singleton class tracks elapsed time for a named level run.
     * When the run ends via Stop(), it fires a callback that game code
     * can use to handle the result (submit to Firebase, save locally,
     * show UI, etc.). The system maintains only timing state and defers
     * all game-specific logic to the callback.
     */
    class TimerSystem {
    public:
        /**
         * @brief Gets the singleton instance of TimerSystem.
         *
         * @return TimerSystem& Reference to the singleton instance
         */
        static TimerSystem& Get() {
            static TimerSystem instance;
            return instance;
        }

        // -------- Timer Control --------
        /**
         * @brief Starts the timer for a specific level.
         *
         * Initializes the timer with zero elapsed time and stores the
         * provided level name. The timer begins accumulating time on
         * subsequent Update() calls.
         *
         * @param levelName The name of the level being timed
         */
        void StartTimer(const std::string& levelName);

        /**
         * @brief Stops the timer and fires the completion callback.
         *
         * If the timer is running, this method:
         * - Stops time accumulation
         * - Invokes the onRunFinished callback (if set) with the level
         *   name and elapsed time
         *
         * @note The callback is invoked on the same thread that calls Stop()
         */
        void Stop();

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
        void Reset();

        /**
         * @brief Updates the timer with frame delta time.
         *
         * If the timer is running, adds the provided delta time to the
         * elapsed time accumulator. Should be called every frame.
         *
         * @param dt Delta time in seconds since the last frame
         */
        void Update(float dt);

        // -------- Timer Queries --------
        /**
         * @brief Checks if the timer is currently running.
         *
         * @return true If the timer is active and accumulating time
         * @return false If the timer is stopped or not started
         */
        bool IsRunning() const { return m_running; }

        /**
         * @brief Gets the current elapsed time.
         *
         * @return float Elapsed time in seconds
         */
        float GetElapsed() const { return m_elapsed; }

        /**
         * @brief Gets the name of the current level being timed.
         *
         * @return const std::string& Reference to the level name
         */
        const std::string& GetLevelName() const { return m_levelName; }

        // -------- Callback Configuration --------
        /**
         * @brief Sets the callback function for run completion events.
         *
         * This callback is invoked inside Stop() when a run ends. Game code
         * should set this once at startup to handle timer completion
         * (Firebase submission, leaderboard display, etc.).
         *
         * @param cb Function taking (levelName, timeSeconds) parameters
         *
         * @note The callback is executed on the main thread.
         */
        void SetOnRunFinished(std::function<void(const std::string& levelName, float timeSeconds)> cb) {
            m_onRunFinished = std::move(cb);
        }

    private:
        /**
         * @brief Private constructor for singleton pattern.
         */
        TimerSystem() = default;

        bool        m_running = false;   //!< Timer running state
        float       m_elapsed = 0.0f;    //!< Accumulated elapsed time in seconds
        std::string m_levelName = "";    //!< Name of current level being timed

        std::function<void(const std::string&, float)> m_onRunFinished; //!< Completion callback
    };

} // namespace Framework