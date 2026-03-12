#pragma once
#include <string>
#include <functional>

namespace Framework {

    // -------------------------------------------------------------------------
    // TimerSystem  (Engine layer - no game-specific dependencies)
    //
    // Tracks elapsed time for a named level run.
    // When the run ends, fires OnRunFinished so game code can decide what
    // to do (submit to Firebase, save locally, show UI, etc.)
    //
    // Usage:
    //   TimerSystem::Get().SetOnRunFinished([](const std::string& level, float t) {
    //       // your game code here
    //   });
    //   TimerSystem::Get().StartTimer("Level_01");
    //   TimerSystem::Get().Stop();   // fires the callback
    // -------------------------------------------------------------------------
    class TimerSystem {
    public:
        static TimerSystem& Get() {
            static TimerSystem instance;
            return instance;
        }

        // --- Control ---
        void StartTimer(const std::string& levelName);
        void Stop();    // Stops the timer and fires OnRunFinished (if set)
        void Reset();
        void Update(float dt);

        // --- Query ---
        bool               IsRunning()   const { return m_running; }
        float              GetElapsed()  const { return m_elapsed; }
        const std::string& GetLevelName() const { return m_levelName; }

        // --- Hook for game code ---
        // Set once at startup from your game-specific submitter script.
        // Called on the main thread inside Stop().
        void SetOnRunFinished(std::function<void(const std::string& levelName, float timeSeconds)> cb) {
            m_onRunFinished = std::move(cb);
        }

    private:
        TimerSystem() = default;

        bool        m_running = false;
        float       m_elapsed = 0.0f;
        std::string m_levelName = "";

        std::function<void(const std::string&, float)> m_onRunFinished;
    };

} // namespace Framework