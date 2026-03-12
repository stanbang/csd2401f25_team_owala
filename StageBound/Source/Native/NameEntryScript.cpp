#include "pch.h"
#include "NameEntryScript.h"
#include "ScriptRegistry.h"
#include "PlayerNameManager.h"
#include "EntityManager.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(NameEntryScript, "NameEntryScript")

// ---------------------------------------------------------------------------
// Helper: map a Win32 virtual-key to a printable ASCII char, respecting Shift.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Debounce helper: returns true only on the FIRST frame a key goes down.
// ---------------------------------------------------------------------------
static bool IsKeyJustPressed(int vk) {
    static std::unordered_map<int, bool> s_wasDown;
    bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool wasDown = s_wasDown[vk];
    s_wasDown[vk] = isDown;
    return isDown && !wasDown;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void NameEntryScript::OnCreate() {
    // Load any previously saved name from disk.
    Framework::PlayerNameManager::Get().Load();

    // Overlay starts hidden ? Open() is called by the Select button script.
    m_active = false;

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), false);
}

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

void NameEntryScript::Cancel() {
    Framework::Logger::Get().Info(DebugType::PlayerSystem)
        << "[NameEntry] Cancelled. Returning to level selection." << std::endl;

    m_active = false;
    m_pendingScene.clear();

    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    mgr.SetActive(GetEntity(), false);
}

void NameEntryScript::RefreshDisplayText() {
    EntityStorage& ecs = GameManager::GetInstance().GetECS();
    EntityManager mgr(ecs);
    Entity self = GetEntity();

    if (!mgr.HasComponent(self, TextComponentType)) return;

    TextComponent& tc = mgr.GetComponent<TextComponent>(self);

    tc.textToDraw = "enter your name:\n" + m_input + "_";
    tc.isDirty = true;
}

void NameEntryScript::OnDestroy() {}