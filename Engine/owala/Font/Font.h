/*****************************************************************//**
 * \file   Font.h
 * \brief
 *
 *
 * \author Yar Xuan
 * \date   26 September 2025
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

        // Loads a font from a file path and generates the texture atlas.
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
}