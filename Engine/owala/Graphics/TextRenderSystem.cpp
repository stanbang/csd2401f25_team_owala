/*********************************************************************//**
 * \file   TextRenderSystem.cpp
 *
 * \primary_author
 *      Yar Xuan
 * \secondary_author
 *
 * \brief
 *      Implements the TextRenderSystem responsible for rendering text.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "TextRenderSystem.h"
#include <cmath>
#include "GameManager.h"
#include "GuiSystem.h"
#include "EntityManager.h"
#include "WindowsSystem.h"

namespace Framework {

    /**
     * \brief Main per-frame update function for text rendering.
     */
    void TextRenderSystem::Update() {
        EntityManager entityMgr(GameManager::GetInstance().GetECS());

        // Loop through all text components
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextComponentType, TransformType, ActiveTagType)) {
            auto& textComp = entityMgr.GetComponent<TextComponent>(entity);
            if (textComp.fontKey.empty() || textComp.textToDraw.empty()) continue;

            // Only calculate if data changed
            if (textComp.isDirty) {
                UpdateWrappedText(textComp);
                UpdateTextBounds(entity, entityMgr, textComp);
                textComp.isDirty = false;
            }
        }
    }

    /**
     * \brief Calculate and update text halfsize 
     */
    void TextRenderSystem::UpdateTextBounds(Entity entity, EntityManager& entityMgr, TextComponent& textComp) {
        ResourceManager& rcm_ = *GameManager::GetInstance().GetSystems().resourceManager_;
        WindowsSystem& win_ = *GameManager::GetInstance().GetSystems().win_;

        try {
            Font& font = rcm_.get<Font>(textComp.fontKey);

            // Find the actual width of text block
            float renderedWidth = 0.0f;
            for (const std::string& line : textComp.wrappedLines) {
                float currentWidth = font.GetStringWidth(line, textComp.scale);
                if (currentWidth > renderedWidth) {
                    renderedWidth = currentWidth;
                }
            }

            // Calculate total rendered height of text block
            float totalHeight = textComp.wrappedLines.size() * font.GetLineHeight(textComp.scale);

            // Total rendered size in world coords
            Vector2D totalSize = Vector2D(renderedWidth, totalHeight);

            // Set halfsize and convert to screen space if is GUI
            if (entityMgr.HasComponent(entity, GUITagType)) {
                Vector2D windowSize = win_.getWindowSize();
                Vector2D screenHalfSize(totalSize.x / windowSize.x, totalSize.y / windowSize.y);

                textComp.halfSize.x = std::abs(screenHalfSize.x);
                textComp.halfSize.y = std::abs(screenHalfSize.y);
            }
            else {
                Vector2D halfSize = totalSize * 0.5f;
                textComp.halfSize.x = std::abs(halfSize.x);
                textComp.halfSize.y = std::abs(halfSize.y);
            }
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::TextRenderSystem)
                << "Failed to update text bounds for Entity: " << entity
                << " (Bad key: " << textComp.fontKey<< "): " << e.what() << std::endl;
        }
    }

    /**
     * \brief Calculate and update text wrappedLines
     */
    void TextRenderSystem::UpdateWrappedText(TextComponent& textComp) {
        ResourceManager& rcm_ = *GameManager::GetInstance().GetSystems().resourceManager_;
        try {
            Font& font = rcm_.get<Font>(textComp.fontKey);

            std::stringstream ss(textComp.textToDraw);
            std::string word;

            std::string currentLine;
            if (ss >> word) currentLine = word;

            std::vector<std::string> lines{};

            unsigned int fontPixelSize = font.GetPixelSize();

            while (ss >> word) {
                std::string testLine = currentLine + " " + word;
                if (font.GetStringWidth(testLine, textComp.scale) > (textComp.maxWidth  * fontPixelSize)) {
                    lines.push_back(currentLine);
                    currentLine = word;
                }
                else {
                    currentLine = testLine;
                }
            }
            lines.push_back(currentLine);

            textComp.wrappedLines = lines;
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::TextRenderSystem)
                << "Failed to update text wrapped lines. (Bad key: " << textComp.fontKey << "): " << e.what() << std::endl;
        }
    }

} // namespace Framework