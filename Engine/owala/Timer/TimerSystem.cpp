#include "pch.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"

namespace Framework {

    void TimerSystem::StartTimer() {
        m_elapsed = 0.0f;
        m_running = true;
    }

    void TimerSystem::Update(float dt) {
        if (m_running)
            m_elapsed += dt;
    }

    void TimerSystem::StopAndSubmit() {
        if (!m_running) return;
        m_running = false;

        // Submit to Firebase on a background thread
        float finalTime = m_elapsed;
        FirebaseManager::Get().SubmitRun(finalTime);
    }

}