#pragma once
#include "ScriptableEntity.h"
#include <string>

using namespace Framework;

// -----------------------------------------------------------------------------
// NameEntryScript
//
// Attach to the name-entry overlay entity in the LevelSelection scene.
// The entity should have a TextComponent for displaying typed input.
//
// Flow:
//   1. SelectLevelButtonScript (or any script) calls NameEntryScript::Open(sceneName)
//      with the scene the player wants to load.
//   2. The overlay becomes visible and captures keyboard input.
//   3. On Enter  -> saves name via PlayerNameManager, then loads the scene.
//   4. On Escape -> cancels, hides overlay, returns control to level selection.
//
// The overlay entity should start with active = false in the scene JSON.
// -----------------------------------------------------------------------------
class NameEntryScript : public ScriptableEntity {
protected:
    void OnCreate()  override;
    void OnUpdate()  override;
    void OnDestroy() override;

public:
    // Call this from the Select button script, passing the target scene name.
    void Open(const std::string& pendingSceneName);

    bool IsActive() const { return m_active; }

private:
    void Confirm();                 // Save name + load pending scene
    void Cancel();                  // Hide overlay, do nothing
    void RefreshDisplayText();      // Push current m_input into TextComponent

    std::string m_input;            // Characters typed so far
    std::string m_pendingScene;     // Scene to load after name is confirmed
    bool        m_active = false;
    bool        m_justOpened = false;

    static constexpr size_t MAX_NAME_LEN = 20;
};