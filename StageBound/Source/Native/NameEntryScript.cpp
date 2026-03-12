/*********************************************************************//**
 * \file   NameEntryScript.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Implementation of the NameEntryScript for capturing player name
 *         before level start.
 *
 * \details
 *      This file contains the implementation of the name entry overlay system.
 *      It handles keyboard input capture, display updates via TextComponent,
 *      and integration with PlayerNameManager for persistence. The script
 *      creates a modal overlay that intercepts keyboard input when active.
 *
 *      Key features implemented:
 *        - Real-time keyboard input capture with Shift key support
 *        - Key debouncing to prevent repeated characters
 *        - Backspace support for deletion
 *        - Enter to confirm (saves name, loads target scene)
 *        - Escape to cancel (returns to level selection)
 *        - Max length enforcement (20 characters)
 *        - Visual feedback with blinking cursor (_)
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "NameEntryScript.h"
#include "ScriptRegistry.h"
#include "PlayerNameManager.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(NameEntryScript, "NameEntryScript")

// ---------------------------------------------------------------------------
// Keyboard Input Helpers
// ---------------------------------------------------------------------------

/**
 * @brief Converts a Windows virtual-key code to a printable ASCII character.
 *
 * Takes into account the current keyboard state (Shift key, etc.) to produce
 * the correct character. Only returns true for printable characters (ASCII 32-126).
 *
 * @param vk Virtual-key code (e.g., VK_A, VK_SPACE)
 * @param outChar Reference to store the resulting character
 * @return true if the key produces a printable character, false otherwise
 *
 * @note This function is Windows-specific and uses Win32 APIs.
 */
    static bool IsPrintable(int vk, char& outChar) {
#ifdef _WIN32
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    BYTE keyState[256] = {};
    GetKeyboardState(keyState);
    char buf[4] = {};
    if (ToAscii(vk, scanCode, keyState, reinterpret_cast<LPWORD>(buf), 0) == 1
        && buf[0] >= 32 && buf[0] <= 126) {
        outChar = buf[0];
        return true;
    }
#endif
    return false;
}

/**
 * @brief Detects if a key was just pressed this frame (edge detection).
 *
 * Maintains a static map of previous key states to detect the exact frame
 * when a key transitions from up to down. Prevents repeated characters from
 * holding down a key.
 *
 * @param vk Virtual-key code to check
 * @return true if the key was just pressed this frame, false otherwise
 */
static bool IsKeyJustPressed(int vk) {
    static std::unordered_map<int, bool> s_wasDown;
    bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool wasDown = s_wasDown[vk];
    s_wasDown[vk] = isDown;
    return isDown && !wasDown;
}

// ---------------------------------------------------------------------------
// Lifecycle Implementation
// ---------------------------------------------------------------------------

/**
 * @brief Initializes the name entry script when the entity is created.
 *
 * Loads any previously saved name from disk and ensures the overlay
 * starts hidden (inactive) until explicitly opened.
 */
void NameEntryScript::OnCreate() {
    // Load any previously saved name from disk.
    Framework::PlayerNameManager::Get().Load();

    // Overlay starts hidden - Open() is called by the Select button script.
    m_active = false;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), false);
}

/**
 * @brief Opens the name entry overlay for a specific target scene.
 *
 * Called by the level selection button script when the player selects a level
 * but needs to enter their name. Pre-fills the input with any existing name
 * from PlayerNameManager so the player can just press Enter to keep it.
 *
 * @param pendingSceneName The scene to load after name confirmation
 */
void NameEntryScript::Open(const std::string& pendingSceneName) {
    m_pendingScene = pendingSceneName;

    // Pre-fill with the existing name so the player can just press Enter
    // to keep it, or start typing to replace it.
    m_input = Framework::PlayerNameManager::Get().GetName();
    m_active = true;
    m_justOpened = true;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), true);

    RefreshDisplayText();

    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[NameEntry] Opened for scene: \"" << pendingSceneName
        << "\". Current name: \"" << m_input << "\"" << std::endl;
}

/**
 * @brief Updates the script each frame, processing keyboard input when active.
 *
 * When active, this method:
 * 1. Skips the first frame after opening (prevents immediate Enter consumption)
 * 2. Checks for Enter (confirm) or Escape (cancel)
 * 3. Processes backspace for deletion
 * 4. Scans all printable keys and appends valid characters
 * 5. Updates the display text when changes occur
 */
void NameEntryScript::OnUpdate() {
    if (!m_active) return;

    // Skip the frame that triggered Open() so we don't immediately consume Enter.
    if (m_justOpened) {
        m_justOpened = false;
        return;
    }

    bool textChanged = false;

#ifdef _WIN32
    if (IsKeyJustPressed(VK_RETURN)) {
        Confirm();
        return;
    }

    if (IsKeyJustPressed(VK_ESCAPE)) {
        Cancel();
        return;
    }

    if (IsKeyJustPressed(VK_BACK)) {
        if (!m_input.empty()) {
            m_input.pop_back();
            textChanged = true;
        }
    }
    else {
        // Scan all possible virtual key codes for printable characters
        for (int vk = 0x20; vk <= 0xFE; ++vk) {
            if (IsKeyJustPressed(vk)) {
                char c = 0;
                if (IsPrintable(vk, c) && m_input.size() < MAX_NAME_LEN) {
                    m_input += c;
                    textChanged = true;
                    break;
                }
            }
        }
    }
#endif

    if (textChanged) {
        RefreshDisplayText();
    }
}

/**
 * @brief Confirms the entered name and loads the target scene.
 *
 * Validates that the name is not empty, saves it via PlayerNameManager,
 * hides the overlay, and initiates level load with fade sequence.
 */
void NameEntryScript::Confirm() {
    // Require at least one character
    if (m_input.empty()) {
        Framework::Logger::Get().Info(DebugType::PlayerSystem)
            << "[NameEntry] Name is empty - please type a name first." << std::endl;
        return; // Keep overlay open
    }

    Framework::PlayerNameManager::Get().SetName(m_input);

    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[NameEntry] Name confirmed: \"" << m_input
        << "\". Loading scene: \"" << m_pendingScene << "\"" << std::endl;

    m_active = false;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), false);

    // Use InitiateLevelLoad to match LevelSelection's fade + load sequence.
    GameManager::GetInstance().InitiateLevelLoad(m_pendingScene);
}

/**
 * @brief Cancels name entry and returns to level selection.
 *
 * Hides the overlay, clears the pending scene, and does not load anything.
 */
void NameEntryScript::Cancel() {
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[NameEntry] Cancelled. Returning to level selection." << std::endl;

    m_active = false;
    m_pendingScene.clear();

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), false);
}

/**
 * @brief Updates the TextComponent to display current input with cursor.
 *
 * Formats the display text as:
 * "enter your name:\n[input]_"
 * where [input] is the current m_input string and _ is a blinking cursor.
 *
 * @note Does nothing if the entity lacks a TextComponent.
 */
void NameEntryScript::RefreshDisplayText() {
    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    Entity self = GetEntity();

    if (!mgr.HasComponent(self, TextComponentType)) return;

    TextComponent& tc = mgr.GetComponent<TextComponent>(self);

    tc.textToDraw = "enter your name:\n" + m_input + "_";
    tc.isDirty = true;
}

/**
 * @brief Cleans up when the entity is destroyed.
 *
 * Empty implementation as no special cleanup is required.
 */
void NameEntryScript::OnDestroy() {}