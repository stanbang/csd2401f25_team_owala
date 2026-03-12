/*********************************************************************//**
 * \file   PlayerNameManager.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Declaration of the PlayerNameManager singleton for persistent storage
 *      and management of player names across game sessions.
 *
 * \details
 *      The PlayerNameManager provides a simple interface for loading, saving,
 *      and managing the player's name. It automatically persists the name to
 *      "player_name.txt" and handles basic sanitization (whitespace trimming,
 *      length limits). The class is implemented as a singleton accessible via
 *      Get().
 *
 *      Key features:
 *        - Automatic loading on first access
 *        - Persistent storage in a text file
 *        - Name sanitization (trimming, max length 20 chars)
 *        - Manual clear functionality
 *
 *      Example Usage:
 *      \code
 *      // Initialize at startup
 *      PlayerNameManager::Get().Load();
 *
 *      // Check if player has a name
 *      if (!PlayerNameManager::Get().HasName()) {
 *          // Prompt player for name
 *          PlayerNameManager::Get().SetName(playerInput);
 *      }
 *
 *      // Use current name
 *      std::string playerName = PlayerNameManager::Get().GetName();
 *
 *      // Clear name (e.g., from settings menu)
 *      PlayerNameManager::Get().Clear();
 *      \endcode
 *
 * \note
 *      The manager uses a simple text file for storage. The save file location
 *      is relative to the working directory of the executable.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include <string>

namespace Framework {

    // ------------------------------------------------------------------
    // Player Name Manager (Singleton)
    // ------------------------------------------------------------------
    /**
     * @brief Manages persistent storage and retrieval of the player's name.
     *
     * This singleton class handles all operations related to the player's name
     * including loading from disk, saving to disk, validation, and in-memory
     * storage. It ensures that the name is properly sanitized before storage
     * and maintains a maximum length of 20 characters.
     *
     * The name is stored in "player_name.txt" next to the executable and
     * persists between game sessions.
     */
    class PlayerNameManager {
    public:
        /**
         * @brief Gets the singleton instance of PlayerNameManager.
         *
         * @return PlayerNameManager& Reference to the singleton instance
         */
        static PlayerNameManager& Get() {
            static PlayerNameManager instance;
            return instance;
        }

        // -------- Name Access --------
        /**
         * @brief Returns the player's chosen name.
         *
         * @return const std::string& The current player name, or empty string if not set
         */
        const std::string& GetName() const { return m_name; }

        /**
         * @brief Checks if the player has already chosen a name.
         *
         * @return true If a non-empty name has been set
         * @return false If no name has been set or name is empty
         */
        bool HasName() const { return !m_name.empty(); }

        // -------- Name Modification --------
        /**
         * @brief Sets a new player name and immediately persists it to disk.
         *
         * Performs sanitization:
         * - Trims leading/trailing whitespace
         * - Enforces maximum length of 20 characters
         * - Automatically saves to disk
         *
         * @param name The new player name (will be cleaned before saving)
         */
        void SetName(const std::string& name);

        // -------- File Operations --------
        /**
         * @brief Loads the player's name from disk.
         *
         * Reads the save file and populates the internal name buffer.
         * Should be called once at startup (e.g., in App::Init).
         */
        void Load();

        /**
         * @brief Clears the current player name and deletes the saved file.
         *
         * Resets the in-memory name to empty and truncates the save file.
         * Useful for "reset name" option in settings.
         */
        void Clear();

    private:
        /**
         * @brief Private constructor for singleton pattern.
         */
        PlayerNameManager() = default;

        static constexpr const char* SAVE_FILE = "player_name.txt"; //!< Path to the save file

        std::string m_name; //!< In-memory player name storage
    };

} // namespace Framework