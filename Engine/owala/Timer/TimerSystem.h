#pragma once
#include <string>
#include <chrono>

namespace Framework {

    class TimerSystem {
    public:
        static TimerSystem& Get() {
            static TimerSystem instance;
            return instance;
        }

        void StartTimer();
        void StopAndSubmit();   // stops timer, submits to Firebase
        void Update(float dt);  // call every frame to accumulate time

        bool  IsRunning()    const { return m_running; }
        float GetElapsed()   const { return m_elapsed; }

    private:
        TimerSystem() = default;

        bool  m_running = false;
        float m_elapsed = 0.0f; // seconds
    };

}