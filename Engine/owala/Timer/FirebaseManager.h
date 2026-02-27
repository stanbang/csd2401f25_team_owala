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
        std::string level;  // Add this field
    };

    class FirebaseManager {
    public:
        static FirebaseManager& Get() {
            static FirebaseManager instance;
            return instance;
        }

        // Submit a run time to Firebase (non-blocking, runs on background thread)
        void SubmitRun(const std::string& levelName, float timeSeconds);

        // Fetch leaderboard for a level (non-blocking, results via callback)
        void FetchLeaderboard(const std::string& levelName,
            std::function<void(std::vector<LeaderboardEntry>)> onComplete);

        // Call this every frame from your game loop to dispatch callbacks to main thread
        void Update();

    private:
        FirebaseManager() = default;

        // Your Firebase Realtime Database URL
        static constexpr const char* FIREBASE_URL =
            "https://stagebound-1547b-default-rtdb.asia-southeast1.firebasedatabase.app/";

        static std::string GetMachineName();

        // Pending callbacks to fire on main thread
        std::mutex m_mutex;
        std::vector<std::function<void()>> m_pendingCallbacks;
    };

}