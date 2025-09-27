/*****************************************************************//**
 * \file   FontManager.cpp
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   26 September 2025
 *********************************************************************/
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