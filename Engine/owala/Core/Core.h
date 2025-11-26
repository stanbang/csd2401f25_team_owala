/*****************************************************************//**
 * \file   Core.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (80%)
 * \secondary_author
 *     Lur Pei Hao Stanley (20%)
 *
 * \brief
 *   Core engine loop and system manager. Handles initialization,
 *   per-frame updates, shutdown, and performance timing of subsystems
 *   derived from ISystem.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include <vector>
#include "ISystem.h"
#include <string>

namespace Framework {

    enum class EngineMode {
        Editor,  // Scene editing mode
        Play     // Runtime gameplay mode
    };

    /**
     * \struct SystemTime
     * \brief Holds performance timing data for a single system.
     *
     * Used for profiling and visualization (e.g., in debug overlays).
     */
    struct SystemTime {
        std::string name;       //!< System name identifier.
        double milliseconds = 0.0; //!< Time spent in Update() this frame.
        float percentage = 0.0f;   //!< Percentage of total frame time.
    };

    /**
     * \class CoreEngine
     * \brief Central engine coordinator. Owns and updates all systems.
     *
     * CoreEngine is responsible for:
     * - Storing a list of ISystem instances
     * - Initializing them in order
     * - Running the game loop (per-frame Update)
     * - Recording timing data for profiling
     * - Shutting down and cleaning up
     *
     * Example usage:
     * \code
     * CoreEngine core;
     * core.AddSystem(new Graphics(winSys));
     * core.AddSystem(new InputSystem(winSys));
     * core.Initialize();
     * core.GameLoop(); // runs until RequestQuit()
     * core.DestroySystems();
     * \endcode
     */
    class CoreEngine {
    public:
        /// \brief Default constructor.
        CoreEngine() = default;

        /// \brief Deleted copy constructor.
        CoreEngine(const CoreEngine&) = delete;

        /// \brief Deleted assignment operator.
        CoreEngine& operator=(const CoreEngine&) = delete;

        /// \brief Default destructor.
        ~CoreEngine() = default;

        /**
         * \brief Add a system to the engine (takes ownership).
         * \param system Pointer to a dynamically allocated ISystem.
         * \return The same pointer for convenience.
         *
         * \details
         * - CoreEngine takes ownership and will delete the system in DestroySystems().
         * - If the engine is already running (initialized_), the new system is immediately
         *   initialized (hot-add).
         */
        ISystem* AddSystem(ISystem* system);

        /**
         * \brief Access per-system timing data from the last frame.
         * \return Const reference to vector of SystemTime entries.
         */
        const std::vector<SystemTime>& GetSystemTimes() const { return systemTimes_; }

        /**
         * \brief Get total frame time (sum of all system updates).
         * \return Total time in milliseconds.
         */
        double GetTotalFrameTime() const { return totalFrameTime_; }

        /**
         * \brief Initialize all systems in order.
         * \details Calls Initialize() on each system in the list.
         */
        void Initialize();

        /**
         * \brief Run the main game loop.
         * \details
         * - Implements a variable-timestep loop.
         * - Each frame calls Update(dt) on all systems in order.
         * - Records timing for profiling.
         * - Continues until RequestQuit() is called.
         */
        void GameLoop();

        /**
         * \brief Shut down and destroy all systems.
         * \details
         * - Calls Shutdown() on all systems in reverse order.
         * - Deletes the system objects (ownership cleanup).
         */
        void DestroySystems();

        /**
         * \brief Signal the game loop to exit.
         * \details Sets running_ to false; GameLoop() will return.
         */
        void RequestQuit() { running_ = false; }

        // --- Engine Mode Management ---
        void SetMode(EngineMode mode) { mode_ = mode; }
        EngineMode GetMode() const { return mode_; }
        bool IsEditorMode() const { return mode_ == EngineMode::Editor; }
        bool IsPlayMode() const { return mode_ == EngineMode::Play; }

    private:
        bool running_ = false;      //!< Whether the game loop is active.
        bool initialized_ = false;  //!< Whether systems have been initialized.
        bool wasPaused_ = false; // To detect state transitions for interrupt handling

        std::vector<ISystem*> systems_; //!< Owned list of engine systems.

        std::vector<SystemTime> systemTimes_; //!< Timing data per system.
        double totalFrameTime_ = 0.0;         //!< Accumulated frame time (ms).

        EngineMode mode_ = EngineMode::Play;  //!< Current engine mode

    };

} // namespace Framework
