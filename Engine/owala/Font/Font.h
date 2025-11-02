/*****************************************************************//**
 * \file   Font.h
 *
 * \primary_author
 *      Yar Xuan (80%)
 * \secondary_author
 *      Lur Pei Hao Stanley (20%)
 *
 * \brief
 *      This file implements the Font class, which handles the loading, 
 *      storage, and unloading of font data.
 *      This file also implements the FreeTypeManager class, a singleton which 
 *      handles the FreeType library instance and ensures only one instance
 *      is initialized throughout.
 * 
 *       
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once

#include <iostream>
#include <string>
#include <map>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <ft2build.h>
#include FT_FREETYPE_H

// Forward declarations for FreeType
struct FT_LibraryRec_;
struct FT_FaceRec_;
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

// A struct to hold a character's metrics and texture coordinates on the atlas.
struct Character {
    glm::ivec2   Size;        // Size of the glyph
    glm::ivec2   Bearing;     // Offset from baseline to left/top of glyph
    unsigned int Advance;     // How far to advance to the next character
    glm::vec4    TexCoords;   // UV coordinates on the texture atlas (x, y, width, height)
};

namespace Framework {
    class Font {
    public:
        Font();
        ~Font();

        // Loads a font from a file path and generates the texture atlas. Return true if successful load, false otherwise.
        bool Load(FT_Library library, const std::string& fontPath, unsigned int size = 48);

        // Get a reference to the character map.
        const std::map<char, Character>& GetCharacters() const;

        // Get the OpenGL texture atlas ID.
        unsigned int GetAtlasID() const;

        // Calculates the width of a string in pixels at a given scale.
        float GetStringWidth(const std::string& text, float scale) const;

    private:
        std::map<char, Character> m_characters;
        unsigned int m_textureAtlas;
        FT_Face m_face;
    };

    // Need this to ensure only one instance of FT_Library is used throughout
    // inefficient to initialize multiple instances of this library
    class FreeTypeManager {
    public:
        static FreeTypeManager& GetInstance() {
            static FreeTypeManager instance;
            return instance;
        }

        // Returns the single global library handle
        FT_Library GetLibrary() const {
            return m_library;
        }

    private:
        FT_Library m_library;

        // Private constructor/destructor ensures only one instance
        FreeTypeManager();
        ~FreeTypeManager();

        // Disable copy/move
        FreeTypeManager(const FreeTypeManager&) = delete;
        FreeTypeManager& operator=(const FreeTypeManager&) = delete;
    };
}