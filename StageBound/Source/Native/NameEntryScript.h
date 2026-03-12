/*********************************************************************//**
 * \file   NameEntryScript.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Name entry overlay script for capturing player name before level start.
 *         Attach to the name-entry overlay entity in the LevelSelection scene.
 *
 * \details
 *      This script manages a text input overlay that appears when the player
 *      selects a level but hasn't set a name yet. It captures keyboard input,
 *      displays the current name being typed, and integrates with PlayerNameManager
 *      to persist the name across sessions.
 *
 *      Key features:
 *        - Modal text input overlay with keyboard capture
 *        - Real-time display of typed characters via TextComponent
 *        - Max length enforcement (20 characters)
 *        - Enter to confirm (saves name and loads target scene)
 *        - Escape to cancel (hides overlay, returns to level selection)
 *        - Automatic integration with PlayerNameManager
 *
 *      Flow:
 * \code
 * // 1. SelectLevelButtonScript (or any script) calls:
 * NameEntryScript::Open("Level1");
 *
 * // 2. Overlay becomes visible and captures keyboard input
 * // 3. On Enter -> saves name via PlayerNameManager, then loads the scene
 * // 4. On Escape -> cancels, hides overlay, returns to level selection
 * \endcode
 *
 * \note
 *      The overlay entity should start with active = false in the scene JSON.
 *      The entity must have a TextComponent to display the current input.
 *
 * \warning
 *      This script expects the entity to have a TextComponent. Without it,
 *      the display will not update.
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
 * @brief Manages a modal name entry overlay for capturing player name before level start.
 *
 * This script handles the entire name entry flow:
 * - Opening the overlay with a target scene name
 * - Capturing keyboard input (letters, numbers, backspace)
 * - Displaying typed characters in real-time via TextComponent
 * - Confirming with Enter (saves name, loads scene)
 * - Canceling with Escape (hides overlay)
 *
 * The overlay is modal - when active, it captures all keyboard input and
 * prevents interaction with underlying UI elements.
 */
class NameEntryScript : public ScriptableEntity {
protected:
    /**
     * @brief Initializes the script when the entity is created.
     *
     * Ensures the overlay starts hidden (if not already set in scene).
     */
    void OnCreate() override;

    /**
     * @brief Updates the script each frame, handling input when active.
     *
     * When active, captures keyboard input:
     * - Alphanumeric keys: append to input string
     * - Backspace: remove last character
     * - Enter: call Confirm()
     * - Escape: call Cancel()
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up when the entity is destroyed.
     */
    void OnDestroy() override;

public:
    /**
     * @brief Opens the name entry overlay for a specific target scene.
     *
     * Call this from the Select button script (or any script) when the player
     * selects a level but needs to enter their name.
     *
     * @param pendingSceneName The scene to load after name is confirmed
     *
     * @note If the player already has a name (PlayerNameManager::HasName()),
     *       this will immediately load the scene without showing the overlay.
     */
    void Open(const std::string& pendingSceneName);

    /**
     * @brief Checks if the overlay is currently active.
     *
     * @return true if overlay is visible and capturing input, false otherwise
     */
    bool IsActive() const { return m_active; }

private:
    /**
     * @brief Confirms the entered name and loads the target scene.
     *
     * - Saves the entered name via PlayerNameManager::SetName()
     * - Hides the overlay
     * - Loads the pending scene
     */
    void Confirm();

    /**
     * @brief Cancels name entry and hides the overlay.
     *
     * - Hides the overlay
     * - Discards entered text
     * - Returns to level selection (no scene load)
     */
    void Cancel();

    /**
     * @brief Updates the entity's TextComponent with the current input string.
     *
     * Called whenever m_input changes to keep the display synchronized.
     */
    void RefreshDisplayText();

    std::string m_input;            //!< Characters typed so far
    std::string m_pendingScene;     //!< Scene to load after name is confirmed
    bool        m_active = false;   //!< Whether overlay is currently visible
    bool        m_justOpened = false; //!< Flag to prevent input from same frame as open

    static constexpr size_t MAX_NAME_LEN = 20; //!< Maximum allowed name length
};