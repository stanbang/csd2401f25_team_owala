/*********************************************************************//**
 * \file   FirebaseManager.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the FirebaseManager that handles cloud-based
 *      leaderboard functionality for the game using Firebase Realtime Database.
 *
 * \details
 *      The FirebaseManager system provides asynchronous communication with
 *      Firebase REST API to submit and retrieve leaderboard entries. It
 *      manages:
 *        - Submitting player run times to the cloud database
 *        - Fetching and sorting leaderboard data for specific levels
 *        - Thread-safe callback management for async operations
 *        - JSON serialization/deserialization using RapidJSON
 *
 *      Internally, the system uses WinHTTP for network requests and processes
 *      responses on background threads, marshaling callbacks to the main thread
 *      via the Update() method. The manager operates as a singleton accessible
 *      via FirebaseManager::Get().
 *
 *      Example Usage:
 *      \code
 *      // Initialize at startup
 *      FirebaseManager::Get().SetDatabaseUrl("https://mygame-12345.firebaseio.com");
 *
 *      // Submit a run
 *      FirebaseManager::Get().SubmitRun("Level1", "Player123", 45.67f);
 *
 *      // Fetch leaderboard
 *      FirebaseManager::Get().FetchLeaderboard("Level1",
 *          [](std::vector<LeaderboardEntry> entries) {
 *              for (const auto& entry : entries) {
 *                  std::cout << entry.name << ": " << entry.time << "s\n";
 *              }
 *          });
 *
 *      // In game loop (main thread):
 *      FirebaseManager::Get().Update(); // Process callbacks on main thread
 *      \endcode
 *
 * \note
 *      This class is thread-safe for its public interface. Network operations
 *      run on detached threads, and callbacks are marshaled to the main thread
 *      via the Update() method.
 *
 * \warning
 *      SetDatabaseUrl() must be called before any other operations, or
 *      submissions will fail with an error logged.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace Framework {

    // ------------------------------------------------------------------
    // Leaderboard entry structure
    // ------------------------------------------------------------------
    /**
     * @brief Represents a single entry in the leaderboard.
     *
     * Contains all the data associated with a player's run on a specific level,
     * including the player name, level name, completion time, and submission timestamp.
     *
     * Entries are sorted by time (ascending) when retrieved from the database.
     */
    struct LeaderboardEntry {
        std::string name;      //!< Player name who achieved the run
        float       time = 0.0f; //!< Completion time in seconds (lower is better)
        long long   timestamp = 0; //!< Unix timestamp when the run was submitted
        std::string level;     //!< Name of the level this entry belongs to
    };

    // ------------------------------------------------------------------
    // Firebase Manager System
    // ------------------------------------------------------------------
    /**
     * @brief Manages asynchronous communication with Firebase Realtime Database.
     *
     * This class handles all Firebase-related operations including submitting
     * new leaderboard entries and retrieving existing ones. It is designed as
     * a singleton accessible via Get().
     *
     * The manager knows nothing about game state - all context (player names,
     * level names, etc.) must be passed explicitly by the calling code. This
     * keeps the engine layer clean and decoupled from game-specific logic.
     *
     * All network operations run on background threads to avoid blocking the
     * main game loop. Results are returned via callbacks that are safely
     * marshaled to the main thread through the Update() method.
     */
    class FirebaseManager {
    public:
        /**
         * @brief Gets the singleton instance of FirebaseManager.
         *
         * @return FirebaseManager& Reference to the singleton instance
         */
        static FirebaseManager& Get() {
            static FirebaseManager instance;
            return instance;
        }

        // -------- Configuration --------
        /**
         * @brief Sets the Firebase Realtime Database URL.
         *
         * Must be called once at startup before any other operations. The URL
         * should point to the root of your Firebase database (e.g.,
         * "https://mygame-12345.firebaseio.com").
         *
         * @param url The Firebase database URL
         *
         * \note This method is not thread-safe and should only be called from
         *       the main thread during initialization.
         */
        void SetDatabaseUrl(const std::string& url) { m_databaseUrl = url; }

        // -------- Leaderboard Operations --------
        /**
         * @brief Submits a new run to the Firebase leaderboard.
         *
         * Creates a new entry in the database under the path:
         * /leaderboard/{levelName}/{playerName}/{timestamp}.json
         *
         * The operation runs asynchronously on a background thread and does
         * not block the caller. No callback is provided for completion as
         * submission failures are logged but not reported back to the game.
         *
         * @param levelName   Name of the level where the run occurred
         * @param playerName  Name of the player who achieved the run
         * @param timeSeconds Completion time in seconds
         *
         * \note If playerName is empty, the submission is silently skipped.
         * \warning If SetDatabaseUrl() hasn't been called, an error is logged.
         */
        void SubmitRun(const std::string& levelName,
            const std::string& playerName,
            float              timeSeconds);

        /**
         * @brief Fetches the leaderboard for a specific level.
         *
         * Retrieves all entries for the given level from the database path:
         * /leaderboard/{levelName}.json
         *
         * The operation runs asynchronously on a background thread. When
         * complete, the results are sorted by time (ascending) and passed to
         * the provided callback function. The callback is guaranteed to be
         * invoked on the main thread when Update() is called.
         *
         * @param levelName   Name of the level to fetch leaderboard for
         * @param onComplete  Callback function that receives the sorted entries
         *
         * \note If the network request fails or the response is empty, the
         *       callback is not invoked.
         * \warning The callback may be invoked multiple frames after the call
         *          to FetchLeaderboard().
         */
        void FetchLeaderboard(const std::string& levelName,
            std::function<void(std::vector<LeaderboardEntry>)> onComplete);

        // -------- Main Thread Processing --------
        /**
         * @brief Processes pending callbacks on the main thread.
         *
         * This method must be called every frame from the game's main loop.
         * It safely dispatches any completed asynchronous operations to their
         * registered callbacks. All callbacks are executed in the order they
         * were received.
         *
         * Example:
         * \code
         * while (running) {
         *     processInput();
         *     updateGame();
         *     FirebaseManager::Get().Update(); // Process Firebase callbacks
         *     render();
         * }
         * \endcode
         */
        void Update();

    private:
        /**
         * @brief Private constructor for singleton pattern.
         */
        FirebaseManager() = default;

        std::string m_databaseUrl;   //!< Firebase database URL (set via SetDatabaseUrl())

        std::mutex m_mutex;          //!< Mutex for thread-safe callback queue access
        std::vector<std::function<void()>> m_pendingCallbacks; //!< Queue of callbacks awaiting execution on main thread
    };

} // namespace Framework