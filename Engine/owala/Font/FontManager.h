/*****************************************************************//**
 * \file   FontManager.h
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

// Need this to ensure only one instance of FT_Library is used throughout
// inefficient to initialize multiple instances of this library

namespace Framework {
    class FontManager {
        public:
            static FontManager& GetInstance() {
                static FontManager instance;
                return instance;
            }

            // Returns the single global library handle
            FT_Library GetLibrary() const {
                return m_library;
            }

        private:
            FT_Library m_library;

            // Private constructor/destructor ensures only one instance
            FontManager();
            ~FontManager();

            // Disable copy/move
            FontManager(const FontManager&) = delete;
            FontManager& operator=(const FontManager&) = delete;
    };
}