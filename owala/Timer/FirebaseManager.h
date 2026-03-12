#pragma once
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace Framework {

    struct LeaderboardEntry {
        std::string name;
        float       time = 0.0f;
        long long   timestamp = 0;
        std::string level;
    };

    // -------------------------------------------------------------------------
    // FirebaseManager  (Engine layer)
    //
    // Handles raw HTTP communication with Firebase Realtime Database.
    // It knows nothing about player names, game state flags, or scene names.
    // All that context is passed in explicitly by the caller (game code).
    //
    // Configure the database URL via SetDatabaseUrl() at startup before use.
    // -------------------------------------------------------------------------
    class FirebaseManager {
    public:
        static FirebaseManager& Get() {
            static FirebaseManager instance;
            return instance;
        }

        // Call once at startup with your project's Realtime Database URL.
        void SetDatabaseUrl(const std::string& url) { m_databaseUrl = url; }

        // Submit a named run. playerName and levelName are provided explicitly
        // by game code - FirebaseManager does not look them up itself.
        void SubmitRun(const std::string& levelName,
            const std::string& playerName,
            float              timeSeconds);

        // Fetch leaderboard for a level (non-blocking, results via callback).
        void FetchLeaderboard(const std::string& levelName,
            std::function<void(std::vector<LeaderboardEntry>)> onComplete);

        // Call every frame from your game loop to dispatch callbacks to main thread.
        void Update();

    private:
        FirebaseManager() = default;

        std::string m_databaseUrl;   // Set via SetDatabaseUrl()

        std::mutex m_mutex;
        std::vector<std::function<void()>> m_pendingCallbacks;
    };

} // namespace Framework