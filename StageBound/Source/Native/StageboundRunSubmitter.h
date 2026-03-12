/*********************************************************************//**
 * \file   StageboundRunSubmitter.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Game-layer script that connects TimerSystem to Firebase for
 *         leaderboard submissions in the Stagebound project.
 *
 * \details
 *      This script serves as the single integration point between the engine's
 *      timer system and the game-specific Firebase leaderboard functionality.
 *      It configures FirebaseManager with the Stagebound project URL and
 *      registers a callback with TimerSystem that handles run completion logic.
 *
 *      Responsibilities:
 *        - Sets the Firebase database URL at startup
 *        - Registers the OnRunFinished callback with TimerSystem
 *        - Checks if the player opted to record (GameStateFlags)
 *        - Retrieves the player's name (PlayerNameManager)
 *        - Submits valid runs to Firebase
 *
 *      This is the ONLY file that needs to know about:
 *        - The Firebase database URL
 *        - GameStateFlags
 *        - PlayerNameManager
 *
 *      The engine-layer files (TimerSystem, FirebaseManager, trigger scripts)
 *      have no knowledge of any of this, maintaining clean separation between
 *      engine and game code.
 *
 *      Example Usage:
 *      \code
 *      // Create a persistent entity in your DDOL scene
 *      Entity services = ecs.CreateEntity();
 *      services.GetProperties().name = "GameServices";
 *
 *      // Attach this script (will survive scene changes)
 *      auto& script = services.AddComponent<ScriptComponent>();
 *      script.Bind<StageboundRunSubmitter>();
 *
 *      // That's it! The script automatically:
 *      // - Sets up Firebase URL
 *      // - Registers the callback
 *      // - Handles all submissions when levels complete
 *      \endcode
 *
 * \note
 *      Attach ONE instance of this script to a persistent entity that lives
 *      for the entire game session (e.g., a "Services" entity in DDOL).
 *      The script has no per-frame work - all logic happens via callback.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ScriptableEntity.h"
#include <string>

using namespace Framework;

/**
 * @brief Game-layer script that bridges TimerSystem and Firebase for leaderboard submissions.
 *
 * This script is the central integration point for leaderboard functionality
 * in the Stagebound project. It configures Firebase, registers callbacks,
 * and implements the game-specific logic for when and how to submit run times.
 *
 * The script is designed to be attached to a persistent entity that never
 * gets destroyed between scenes, ensuring the callback remains registered
 * throughout the entire game session.
 *
 * Flow:
 * 1. OnCreate(): Sets Firebase URL and registers callback with TimerSystem
 * 2. When a level completes, TimerSystem calls the callback
 * 3. Callback checks player preferences and name, then submits to Firebase
 *
 * No per-frame work is needed - all logic is event-driven via the callback.
 */
class StageboundRunSubmitter : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the submitter when the entity is created.
     *
     * Performs one-time setup:
     * - Sets the Firebase database URL for Stagebound
     * - Registers the run completion callback with TimerSystem
     */
    void OnCreate() override;

    /**
     * @brief Cleans up when the entity is destroyed.
     *
     * Intentionally does NOT clear the TimerSystem callback because:
     * - The callback captures only singletons (always valid)
     * - The callback needs to survive scene changes
     * - This entity should never be destroyed during gameplay
     */
    void OnDestroy() override;

    /**
     * @brief No per-frame work needed.
     *
     * All logic happens via the TimerSystem callback when runs complete.
     */
    void OnUpdate() override {}
};