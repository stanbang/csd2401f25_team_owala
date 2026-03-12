#include "pch.h"
#include "TimerSystem.h"
#include "Logger.h"

namespace Framework {

    void TimerSystem::StartTimer(const std::string& levelName) {
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerSystem] STARTED timer for level: " << levelName << std::endl;
        OutputDebugStringA(("[TimerSystem] STARTED timer for level: " + levelName + "\n").c_str());

        m_elapsed = 0.0f;
        m_running = true;
        m_levelName = levelName;
    }

    void TimerSystem::Update(float dt) {
        if (m_running)
            m_elapsed += dt;
    }

    void TimerSystem::Stop() {
        if (!m_running) return;
        m_running = false;

        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[TimerSystem] STOPPED. Level: " << m_levelName
            << "  Time: " << m_elapsed << "s" << std::endl;

        // Notify game code - no knowledge of Firebase or player names here.
        if (m_onRunFinished)
            m_onRunFinished(m_levelName, m_elapsed);
    }

    void TimerSystem::Reset() {
        m_running = false;
        m_elapsed = 0.0f;
        m_levelName = "";
    }

} // namespace Framework