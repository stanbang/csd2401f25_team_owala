/*********************************************************************//**
 * \file   StageboundRunSubmitter.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Implementation of the StageboundRunSubmitter script that connects
 *         TimerSystem to Firebase for leaderboard submissions.
 *
 * \details
 *      This script serves as the game-specific glue between the engine's
 *      TimerSystem and FirebaseManager. It configures the Firebase database
 *      URL for Stagebound and registers a callback with TimerSystem that
 *      handles run completion logic including:
 *        - Checking if the player opted to record their time
 *        - Retrieving the player's name from PlayerNameManager
 *        - Submitting valid runs to Firebase
 *        - Logging all steps for debugging
 *
 *      The script is designed to be attached to a persistent entity so that
 *      the callback remains registered across scene changes.
 *
 * \note
 *      This is the ONLY place where game-specific logic (player name,
 *      record flags) meets the engine's timer system. All other systems
 *      remain game-agnostic.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

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

/**
 * @brief Initializes the submitter when the entity is created.
 *
 * Performs two critical setup tasks:
 * 1. Sets the Firebase database URL for the Stagebound project
 * 2. Registers a callback with TimerSystem that handles run completion
 *
 * The callback implements the complete submission logic:
 * - Logs the run completion
 * - Checks if player opted to record (GameStateFlags::shouldRecordTime)
 * - Verifies player has a name set
 * - Submits to Firebase if all conditions are met
 * - Logs each step for debugging
 */
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

            // Check if player opted out
            if (!GameStateFlags::shouldRecordTime) {
                Framework::Logger::Get().Info(DebugType::PlayerSystem)
                    << "[StageboundSubmitter] Player opted out - skipping Firebase." << std::endl;
                return;
            }

            std::string playerName = Framework::PlayerNameManager::Get().GetName();

            // Check if player has a name set
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

/**
 * @brief Cleans up when the entity is destroyed.
 *
 * Intentionally does NOT clear the TimerSystem callback because:
 * - The lambda only captures singletons (FirebaseManager, PlayerNameManager,
 *   GameStateFlags) which remain valid
 * - The callback needs to persist across scene changes
 * - This script is designed to be on a persistent entity that never gets destroyed
 *
 * If this entity is destroyed, the callback will continue to function safely
 * as it has no dependencies on the script instance.
 */
void StageboundRunSubmitter::OnDestroy() {
    // Intentionally NOT clearing the callback here.
    // The lambda only captures singletons (FirebaseManager, PlayerNameManager,
    // GameStateFlags) so it never dangles, and we need it to survive scene loads.
}