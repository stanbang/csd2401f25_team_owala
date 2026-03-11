#include "pch.h"
#include "TimerSystem.h"
#include "FirebaseManager.h"
#include "Logger.h"
namespace Framework {

    void TimerSystem::StartTimer(const std::string& levelName) {
        // Use your existing logger system
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerSystem] STARTED timer for level: " << levelName << std::endl;

        // Also keep OutputDebugString as backup
        OutputDebugStringA(("[TimerSystem] STARTED timer for level: " + levelName + "\n").c_str());

        m_elapsed = 0.0f;
        m_running = true;
        m_levelName = levelName;
    }

    void TimerSystem::Update(float dt) {
        // Log every update
        std::string updateMsg = "[TimerSystem] Update called, running: " + std::to_string(m_running) +
            ", elapsed: " + std::to_string(m_elapsed + dt) + "\n";
        OutputDebugStringA(updateMsg.c_str());

        if (m_running) {
            m_elapsed += dt;
        }
    }
    void TimerSystem::Reset() {
        m_running = false;
        m_elapsed = 0.0f;
        m_levelName = "";
    }

    void TimerSystem::StopAndSubmit() {
        if (!m_running) return;
        m_running = false;

        float finalTime = m_elapsed;
        std::string level = m_levelName;
        FirebaseManager::Get().SubmitRun(level, finalTime);
    }
}