/*********************************************************************//**
 * \file   PlayerNameManager.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief
 *      Implementation of the PlayerNameManager class for persistent storage
 *      of player names across game sessions.
 *
 * \details
 *      This file contains the implementation of all PlayerNameManager methods,
 *      handling file I/O operations for reading and writing the player's name
 *      to a persistent text file. Key features include:
 *        - Loading saved player names from disk on startup
 *        - Saving player names with automatic whitespace trimming
 *        - Enforcing maximum name length constraints
 *        - Clearing saved data when needed
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "PlayerNameManager.h"

#include <fstream>
#include <algorithm>
#include <cctype>

namespace Framework {

    // -----------------------------------------------------------------------

    /**
     * @brief Loads the player's name from persistent storage.
     *
     * Attempts to read the player's previously saved name from SAVE_FILE.
     * If the file exists, reads the first line and removes any trailing
     * whitespace or control characters (CR, LF, spaces).
     */
    void PlayerNameManager::Load() {
        std::ifstream file(SAVE_FILE);
        if (file.is_open()) {
            std::getline(file, m_name);
            // Strip any trailing whitespace / CR
            while (!m_name.empty() && (m_name.back() == '\r' || m_name.back() == '\n' || m_name.back() == ' '))
                m_name.pop_back();
        }
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Sets a new player name and persists it to disk.
     *
     * Performs validation and cleanup on the provided name:
     * - Trims leading and trailing whitespace
     * - Enforces a maximum length of 20 characters
     * - Persists the cleaned name to the save file
     *
     * If the name consists entirely of whitespace after trimming, it becomes empty.
     *
     * @param name The new player name to set
     */
    void PlayerNameManager::SetName(const std::string& name) {
        // Trim leading/trailing whitespace
        m_name = name;
        size_t start = m_name.find_first_not_of(" \t\r\n");
        size_t end = m_name.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            m_name.clear();
        }
        else {
            m_name = m_name.substr(start, end - start + 1);
        }

        // Enforce a max length of 20 characters
        if (m_name.size() > 20)
            m_name.resize(20);

        // Persist to disk
        std::ofstream file(SAVE_FILE, std::ios::trunc);
        if (file.is_open())
            file << m_name;
    }

    // -----------------------------------------------------------------------

    /**
     * @brief Clears the current player name and deletes the saved file.
     *
     * Resets the in-memory player name to empty and truncates the save file
     * to zero bytes, removing any previously saved name.
     */
    void PlayerNameManager::Clear() {
        m_name.clear();
        std::ofstream file(SAVE_FILE, std::ios::trunc); // truncate = empty file
    }

} // namespace Framework