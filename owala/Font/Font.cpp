/*****************************************************************//**
 * \file   Font.cpp
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
#include "pch.h"
#include "Font.h"
#include "Logger.h"

using namespace Framework;

// FreeTypeManager Class
FreeTypeManager::FreeTypeManager() {
    if (FT_Init_FreeType(&m_library)) {
        Logger::Error(DebugType::Font) << "FreeType: Could not init FreeType library" << std::endl;
    }
}

FreeTypeManager::~FreeTypeManager() {
    if (m_library) {
        FT_Done_FreeType(m_library);
        Logger::Info(DebugType::Font) << "Freed FreeType Library" << std::endl;
    }
}

// Font Class
Font::Font() : m_textureAtlas(0), m_characters(), m_face(), m_rawLineHeight(0.f), m_rawAscent(0.f), m_size(0){}

Font::~Font() {
    if (m_textureAtlas) {
        glDeleteTextures(1, &m_textureAtlas);
    }
    if (m_face) {
        FT_Done_Face(m_face);
    }
}

bool Font::Load(FT_Library library, const std::string& fontPath, unsigned int size) {
    if (FT_New_Face(library, fontPath.c_str(), 0, &m_face)) {
        Logger::Error(DebugType::Font) << "FreeType: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    // Set font size
    if (FT_Set_Pixel_Sizes(m_face, 0, size)) {
        Logger::Error(DebugType::Font) << "FreeType: Failed to set font size." << std::endl;
        m_face = nullptr;
        return false;
    }

    m_size = size;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Create a single texture atlas
    glGenTextures(1, &m_textureAtlas);
    glBindTexture(GL_TEXTURE_2D, m_textureAtlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int ATLAS_WIDTH = 1024;
    const int ATLAS_HEIGHT = 1024;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);

    int xOffset = 0;
    int yOffset = 0;
    int rowHeight = 0;

    for (unsigned char c = 32; c <= 126; ++c) {
        if (FT_Load_Char(m_face, c, FT_LOAD_RENDER)) {
            Logger::Error(DebugType::Font) << "FreeType: Failed to load glyph for character " << c << std::endl;
            continue;
        }

        if (xOffset + m_face->glyph->bitmap.width > ATLAS_WIDTH) {
            xOffset = 0;
            yOffset += rowHeight + 2; // Add padding
            rowHeight = 0;
        }

        rowHeight = std::max(rowHeight, static_cast<int>(m_face->glyph->bitmap.rows));
        if (yOffset + rowHeight > ATLAS_HEIGHT) {
            Logger::Error(DebugType::Font) << "Atlas size is too small!" << std::endl;
            break;
        }

        // Skip any glyph that is too wide to fit in the atlas at all
        if (static_cast<int>(m_face->glyph->bitmap.width) > ATLAS_WIDTH) {
            Logger::Error(DebugType::Font) << "Glyph for char " << static_cast<int>(c) << " exceeds atlas width — skipping." << std::endl;
            continue;
        }

        // Copy the glyph bitmap to the texture atlas
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            xOffset,
            yOffset,
            m_face->glyph->bitmap.width,
            m_face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            m_face->glyph->bitmap.buffer
        );


        // Store character metrics and UV coordinates
        Character character = {
            glm::ivec2(m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows),
            glm::ivec2(m_face->glyph->bitmap_left, m_face->glyph->bitmap_top),
            static_cast<unsigned int>(m_face->glyph->advance.x),
            glm::vec4(
                static_cast<float>(xOffset) / ATLAS_WIDTH,
                static_cast<float>(yOffset) / ATLAS_HEIGHT,
                static_cast<float>(m_face->glyph->bitmap.width) / ATLAS_WIDTH,
                static_cast<float>(m_face->glyph->bitmap.rows) / ATLAS_HEIGHT
            )
        };
        m_characters[c] = character;
        xOffset += m_face->glyph->bitmap.width + 2; // Add padding
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    // store raw line height
    FT_Size_Metrics& metrics = m_face->size->metrics;
    m_rawLineHeight = static_cast<float>(metrics.height >> 6);
    m_rawAscent = static_cast<float>(metrics.ascender >> 6);

    return true;
}


const std::map<char, Character>& Font::GetCharacters() const {
    return m_characters;
}

unsigned int Font::GetAtlasID() const {
    return m_textureAtlas;
}

float Font::GetStringWidth(const std::string& text, float scale) const {
    float width = 0.0f;
    for (char c_signed : text) {
        unsigned char c = static_cast<unsigned char>(c_signed);
        if (m_characters.count(c)) {
            const Character& ch = m_characters.at(c);
            // Advance is in 1/64ths of a pixel, so we divide by 64.
            width += (ch.Advance >> 6) * scale;
        }
    }
    return width;
}

float Font::GetLineHeight(float scale) const {
    return m_rawLineHeight * scale;
}

unsigned int Font::GetPixelSize() const {
    return m_size;
}

float Font::GetAscent(float scale) const {
    return m_rawAscent * scale;
}