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
    };

    class FirebaseManager {
    public:
        static FirebaseManager& Get() {
            static FirebaseManager instance;
            return instance;
        }

        // Submit a run time to Firebase (non-blocking, runs on background thread)
        void SubmitRun(float timeSeconds);

        // Fetch leaderboard from Firebase (non-blocking)
        // Calls onComplete on the main thread next Update()
        void FetchLeaderboard(std::function<void(std::vector<LeaderboardEntry>)> onComplete);

        // Call this every frame from your game loop to dispatch callbacks to main thread
        void Update();

    private:
        FirebaseManager() = default;

        // Your Firebase Realtime Database URL — change this to yours
        static constexpr const char* FIREBASE_URL =
            "https://YOUR-PROJECT-default-rtdb.firebaseio.com";

        static std::string GetMachineName();

        // Pending callbacks to fire on main thread
        std::mutex m_mutex;
        std::vector<std::function<void()>> m_pendingCallbacks;
    };

}