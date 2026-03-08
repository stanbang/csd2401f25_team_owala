#pragma once
#include <string>

namespace Framework {

    // -------------------------------------------------------------------------
    // PlayerNameManager
    //
    // Stores and persists the player's chosen display name.
    // The name is saved to "player_name.txt" next to the executable so it
    // survives between sessions.
    //
    // Usage:
    //   PlayerNameManager::Get().GetName()       -> current name (may be empty)
    //   PlayerNameManager::Get().SetName("Ace")  -> save + persist
    //   PlayerNameManager::Get().HasName()       -> false if never set
    // -------------------------------------------------------------------------
    class PlayerNameManager {
    public:
        static PlayerNameManager& Get() {
            static PlayerNameManager instance;
            return instance;
        }

        // Returns the player's chosen name, or "" if not yet set.
        const std::string& GetName() const { return m_name; }

        // Returns true if the player has already chosen a name.
        bool HasName() const { return !m_name.empty(); }

        // Set and immediately persist the name to disk.
        void SetName(const std::string& name);

        // Load name from disk (call once at startup, e.g. in your App::Init).
        void Load();

        // Clear the saved name (useful for "reset name" option).
        void Clear();

    private:
        PlayerNameManager() = default;

        static constexpr const char* SAVE_FILE = "player_name.txt";

        std::string m_name;
    };

} // namespace Framework