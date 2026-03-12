/*********************************************************************//**
 * \file   GameConfig.h
 *
 * \primary_author
 *      Ng Jing Wen (100%)
 *
 * \brief  Lightweight header containing the GameConfig structure for
 *         game-wide configuration settings.
 *
 * \details
 *      This header defines the GameConfig structure used by GameManager
 *      to store runtime configuration settings loaded from JSON. It is
 *      kept lightweight and separate from GameManager.h to allow low-level
 *      systems (e.g., FileIOSystem) to reference the struct without
 *      pulling in the full GameManager header, avoiding circular includes.
 *
 *      The configuration includes:
 *        - Startup scene: First scene to load when the game launches
 *        - Loading scene: Scene to display during asynchronous level loads
 *        - Overlay scenes: UI scenes that pause physics (pause menu, options)
 *        - Menu scenes: Non-level scenes for IsLevelScene() detection
 *
 *      Example JSON configuration (Data/GameConfig.json):
 *      \code
 *      {
 *          "startupScene": "MainMenu",
 *          "loadingScene": "LoadingScreen",
 *          "overlayScenes": ["PauseMenu", "OptionsMenu"],
 *          "menuScenes": ["MainMenu", "LevelSelection", "Credits"]
 *      }
 *      \endcode
 *
 * \note
 *      This header contains only the struct definition and necessary
 *      STL includes. No function implementations or heavy dependencies.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>
#include <vector>

namespace Framework {
    /**
     * @brief Configuration settings for the game, loaded from JSON.
     *
     * This struct holds all runtime configuration values that control
     * game flow, scene management, and physics behavior. It is populated
     * by FileIOSystem::LoadGameConfig() from Data/GameConfig.json.
     */
    struct GameConfig {
        /**
         * @brief Name of the first scene to load when the game starts.
         *
         * Example: "MainMenu", "SplashScreen"
         */
        std::string startupScene = "";

        /**
         * @brief Name of the scene to display during asynchronous level loads.
         *
         * This scene is shown while the target level loads in the background.
         * Example: "LoadingScreen"
         */
        std::string loadingScene = "";

        /**
         * @brief List of scene names that should pause physics when loaded.
         *
         * These are typically UI overlays like pause menus or options screens
         * that should freeze gameplay while visible.
         * Example: ["PauseMenu", "OptionsMenu"]
         */
        std::vector<std::string> overlayScenes;

        /**
         * @brief List of scene names that are NOT playable levels.
         *
         * Used by GameManager::IsLevelScene() to distinguish between actual
         * gameplay levels and menu/UI scenes. This affects timer behavior,
         * leaderboard submissions, and other level-specific logic.
         * Example: ["MainMenu", "LevelSelection", "Credits"]
         */
        std::vector<std::string> menuScenes;
    };
}