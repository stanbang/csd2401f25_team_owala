#pragma once
#include <string>

namespace Framework {

    class TimerSystem {
    public:
        static TimerSystem& Get() {
            static TimerSystem instance;
            return instance;
        }

        void StartTimer(const std::string& levelName);
        void StopAndSubmit();
        void Update(float dt);
        void Reset();

        bool  IsRunning()       const { return m_running; }
        float GetElapsed()      const { return m_elapsed; }
        const std::string& GetLevelName() const { return m_levelName; }

    private:
        TimerSystem() = default;

        bool        m_running = false;
        float       m_elapsed = 0.0f;
        std::string m_levelName = "";
    };
}