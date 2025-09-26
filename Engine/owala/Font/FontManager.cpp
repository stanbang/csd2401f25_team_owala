#include "FontManager.h"
#include <iostream>

namespace Framework {
    FontManager::FontManager() {
        if (FT_Init_FreeType(&m_library)) {
            std::cerr << "FreeType: Could not init FreeType library" << std::endl;
        }
    }

    FontManager::~FontManager() {
        if (m_library) {
            FT_Done_FreeType(m_library);
        }
    }
}