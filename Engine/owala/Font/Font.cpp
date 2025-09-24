#include "Font.h"
#include <iostream>
#include <ft2build.h>
#include FT_FREETYPE_H

Font::Font()
    : m_textureAtlas(0), m_characters() {
    std::cout << "Loaded Font Class" << std::endl;
    if (FT_Init_FreeType(&m_ft)) {
        std::cerr << "FreeType: Could not init FreeType library" << std::endl;
    }
}

Font::~Font() {
    if (m_textureAtlas) {
        std::cout << "Font class deleted m_textureAtlas" << std::endl;
        glDeleteTextures(1, &m_textureAtlas);
    }
    if (m_ft) {
        std::cout << "Font class called FT_Done_FreeType" << std::endl;
        FT_Done_FreeType(m_ft);
    }
}

bool Font::Load(const std::string& fontPath
    //, unsigned int size
) {
    FT_Face face;
    if (FT_New_Face(m_ft, fontPath.c_str(), 0, &face)) {
        std::cerr << "FreeType: Failed to load font: " << fontPath << std::endl;
        return false;
    }

    // Set font size
    FT_Set_Pixel_Sizes(face, 0, 48);
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
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "FreeType: Failed to load glyph for character " << c << std::endl;
            continue;
        }

        if (xOffset + face->glyph->bitmap.width > ATLAS_WIDTH) {
            xOffset = 0;
            yOffset += rowHeight + 2; // Add padding
            rowHeight = 0;
        }

        rowHeight = std::max(rowHeight, static_cast<int>(face->glyph->bitmap.rows));
        if (yOffset + rowHeight > ATLAS_HEIGHT) {
            std::cerr << "Atlas size is too small!" << std::endl;
            break;
        }

        // Copy the glyph bitmap to the texture atlas
        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            xOffset,
            yOffset,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );


        // Store character metrics and UV coordinates
        Character character = {
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            static_cast<unsigned int>(face->glyph->advance.x),
            glm::vec4(
                static_cast<float>(xOffset) / ATLAS_WIDTH,
                static_cast<float>(yOffset) / ATLAS_HEIGHT,
                static_cast<float>(face->glyph->bitmap.width) / ATLAS_WIDTH,
                static_cast<float>(face->glyph->bitmap.rows) / ATLAS_HEIGHT
            )
        };
        m_characters[c] = character;
        xOffset += face->glyph->bitmap.width + 2; // Add padding
    }

    FT_Done_Face(face);
    glBindTexture(GL_TEXTURE_2D, 0);

    //std::cout << "m_characters size " << m_characters.size() << std::endl;
    //for (const auto& pair : m_characters) {
    //    std::cout << "Loaded char: " << static_cast<char>(pair.first) << " (int: " << static_cast<int>(pair.first) << ")" << std::endl;
    //}

    return true;
}


const std::map<char, Character>& Font::GetCharacters() const {
    //std::cout << "get m_characters, size: " << m_characters.size() << std::endl;
    return m_characters;
}


unsigned int Font::GetAtlasID() const {
    return m_textureAtlas;
}

