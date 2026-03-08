#include "pch.h"
#include "PlayerNameManager.h"

#include <fstream>
#include <algorithm>
#include <cctype>

namespace Framework {

    void PlayerNameManager::Load() {
        std::ifstream file(SAVE_FILE);
        if (file.is_open()) {
            std::getline(file, m_name);
            // Strip any trailing whitespace / CR
            while (!m_name.empty() && (m_name.back() == '\r' || m_name.back() == '\n' || m_name.back() == ' '))
                m_name.pop_back();
        }
    }

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

    void PlayerNameManager::Clear() {
        m_name.clear();
        std::ofstream file(SAVE_FILE, std::ios::trunc); // truncate = empty file
    }

} // namespace Framework