/*********************************************************************//**
 * \file   GameStateFlags.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Global state flags for controlling gameplay features and player
 *      preferences across the game.
 *
 * \details
 *      This header provides a simple namespace-based global state management
 *      system for runtime flags that control gameplay behavior. It is designed
 *      for lightweight, cross-cutting concerns that need to be accessed from
 *      multiple systems without complex dependency injection.
 *
 *      Currently supports:
 *        - Leaderboard recording preference (whether to submit runs)
 *        - Current level tracking for leaderboard submissions
 *        - Centralized reset functionality for clean state transitions
 *
 *      Example Usage:
 *      \code
 *      // In LevelSelection UI when player responds to "Record time? (Y/N)"
 *      if (playerSelectedYes) {
 *          GameStateFlags::shouldRecordTime = true;
 *      }
 *
 *      // When loading a level
 *      GameStateFlags::currentLevelName = "Level1";
 *
 *      // In gameplay when player finishes level
 *      if (GameStateFlags::shouldRecordTime) {
 *          FirebaseManager::Get().SubmitRun(
 *              GameStateFlags::currentLevelName,
 *              playerName,
 *              finishTime
 *          );
 *      }
 *
 *      // When leaving gameplay (back to menu or level selection)
 *      GameStateFlags::Reset();
 *      \endcode
 *
 * \note
 *      These flags are global variables. While convenient for simple state
 *      sharing, they should be used judiciously to avoid hidden dependencies
 *      and maintain code clarity.
 *
 * \warning
 *      Not thread-safe. All access should occur from the main game thread.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>

 /**
  * @brief Namespace containing global game state flags for runtime configuration.
  *
  * This namespace holds simple global flags that control various game behaviors
  * across different systems. The flags are implemented as inline variables
  * (C++17) for header-only definition without requiring a separate source file.
  */
namespace GameStateFlags {

    // ------------------------------------------------------------------
    // Leaderboard Recording Control
    // ------------------------------------------------------------------
    /**
     * @brief Controls whether the player's run time should be recorded.
     *
     * This flag is typically set in the level selection screen when the player
     * responds to a prompt asking whether they want their time recorded for
     * the leaderboard. If true, the game will submit the run time to Firebase
     * upon level completion.
     *
     * Default: false
     *
     * @note Reset to false when leaving gameplay via Reset().
     */
    inline bool shouldRecordTime = false;

    // ------------------------------------------------------------------
    // Level Context Tracking
    // ------------------------------------------------------------------
    /**
     * @brief Name of the currently active level being played.
     *
     * This string stores the identifier of the level currently being played,
     * used when submitting run times to the leaderboard. It should be set
     * when the level loads and cleared when leaving gameplay.
     *
     * Example values: "Level1", "Tutorial", "BossBattle"
     *
     * @note Cleared to empty string when leaving gameplay via Reset().
     */
    inline std::string currentLevelName = "";

    // ------------------------------------------------------------------
    // State Management
    // ------------------------------------------------------------------
    /**
     * @brief Resets all game state flags to their default values.
     *
     * This function should be called when transitioning away from gameplay
     * states (e.g., returning to main menu, level selection, or restarting).
     * It ensures that flags from previous play sessions don't affect new ones.
     *
     * Reset behavior:
     * - shouldRecordTime → false
     * - currentLevelName → empty string
     *
     * Example usage:
     * \code
     * void GameplaySystem::ExitToMenu() {
     *     // Clean up gameplay systems
     *     UnloadLevel();
     *
     *     // Reset global flags
     *     GameStateFlags::Reset();
     *
     *     // Transition to menu
     *     SwitchToMenuScene();
     * }
     * \endcode
     */
    inline void Reset() {
        shouldRecordTime = false;
        currentLevelName.clear();
    }

} // namespace GameStateFlags