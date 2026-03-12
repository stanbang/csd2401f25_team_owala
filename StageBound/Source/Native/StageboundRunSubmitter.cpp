#include "pch.h"
#include "StageboundRunSubmitter.h"
#include "ScriptRegistry.h"

// Engine systems
#include "TimerSystem.h"
#include "FirebaseManager.h"
#include "PlayerNameManager.h"
#include "Logger.h"

// Game-specific state
#include "GameStateFlags.h"

REGISTER_SCRIPT(StageboundRunSubmitter, "StageboundRunSubmitter")

// ---------------------------------------------------------------------------
// Database URL for Stagebound's Firebase project.
// ---------------------------------------------------------------------------
static constexpr const char* STAGEBOUND_FIREBASE_URL =
"https://stagebound-1547b-default-rtdb.asia-southeast1.firebasedatabase.app/";

// ---------------------------------------------------------------------------

void StageboundRunSubmitter::OnCreate() {
    // 1. Point FirebaseManager at this game's database.
    Framework::FirebaseManager::Get().SetDatabaseUrl(STAGEBOUND_FIREBASE_URL);

    // 2. Register the run-finished callback on TimerSystem.
    //    This lambda is the single place that decides whether to submit.
    Framework::TimerSystem::Get().SetOnRunFinished(
        [](const std::string& levelName, float timeSeconds)
        {
            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "[StageboundSubmitter] Run finished - level: " << levelName
                << "  time: " << timeSeconds << "s" << std::endl;

            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "[StageboundSubmitter] shouldRecordTime = "
                << (GameStateFlags::shouldRecordTime ? "true" : "false") << std::endl;

            if (!GameStateFlags::shouldRecordTime) {
                Framework::Logger::Get().Info(DebugType::PlayerSystem)
                    << "[StageboundSubmitter] Player opted out - skipping Firebase." << std::endl;
                return;
            }

            std::string playerName = Framework::PlayerNameManager::Get().GetName();

            if (playerName.empty()) {
                Framework::Logger::Get().Info(DebugType::PlayerSystem)
                    << "[StageboundSubmitter] Player name not set - skipping Firebase." << std::endl;
                return;
            }

            Framework::Logger::Get().Info(DebugType::PlayerSystem)
                << "[StageboundSubmitter] Submitting: player='" << playerName
                << "'  level='" << levelName
                << "'  time=" << timeSeconds << "s" << std::endl;

            // Pass everything explicitly - FirebaseManager knows nothing about
            // PlayerNameManager or GameStateFlags.
            Framework::FirebaseManager::Get().SubmitRun(levelName, playerName, timeSeconds);
        }
    );

    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[StageboundSubmitter] Initialised." << std::endl;
}

void StageboundRunSubmitter::OnDestroy() {
    // Intentionally NOT clearing the callback here.
    // The lambda only captures singletons (FirebaseManager, PlayerNameManager,
    // GameStateFlags) so it never dangles, and we need it to survive scene loads.
}