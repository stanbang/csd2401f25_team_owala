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

namespace Framework {

    /**
     * \brief Main per-frame update function for text rendering.
     */
    void TextRenderSystem::Update() {
        EntityManager entityMgr(ecs_);

        // Retrieve active camera
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;
        if (activeCamera != 0 && cameraSystem_) {
            cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        }

        // Loop through all text components
        for (const Entity& entity : entityMgr.GetAllEntitiesWithComponents(TextComponentType, PositionType, ActiveTagType)) {
            auto& textComp = entityMgr.GetComponent<TextComponent>(entity);
            if (textComp.fontKey.empty() || textComp.textToDraw.empty()) continue;

            // Only calculate if data changed
            if (textComp.isDirty) {
                UpdateWrappedText(textComp);
                UpdateTextBounds(entity, entityMgr, textComp, activeCamera);
                textComp.isDirty = false;
            }

            RenderText(entity, entityMgr, textComp, activeCamera);
        }
    }

    /**
     * \brief Renders all visible text components.
     */
    void TextRenderSystem::RenderText(const Entity en, EntityManager& entityMgr, const TextComponent& textComp, const Entity camera) {
        try {
            Font& font = rcm_.get<Font>(textComp.fontKey);
            const Vector2D& textPos = entityMgr.GetComponent<Position>(en).Pos;
            bool isUIElement = entityMgr.HasComponent(en, GUITagType);

            Vector2D screenTextPos = textPos;
            // Texts with GUI tag already use screen coords
            if (camera != 0 && cameraSystem_ && !isUIElement) {
                screenTextPos = cameraSystem_->WorldToScreen(textPos, camera);
            }

            // offset so that pos is the middle of the text
            screenTextPos.x -= textComp.halfSize.x;
            screenTextPos.y += textComp.halfSize.y;

            // get line height for offsetting the y when rendering multiple lines of text
            const float line_height_px = font.GetLineHeight(textComp.scale);
            float line_height_ndc = (line_height_px / gfx_.getScreenHeight()) * 2.0f;

            float zoom = 1.0f;
            if (camera != 0 && cameraSystem_ && !isUIElement) {
                const Camera& cam = cameraSystem_->GetCamera(camera);
                zoom = cam.zoom;
            }

            // draw each line of text
            for (const std::string& line : textComp.wrappedLines) {
                gfx_.drawText(font, line, screenTextPos, textComp.scale / zoom, textComp.color);
                screenTextPos.y -= line_height_ndc / zoom;
            }
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::GraphicsSystem)
                << "Failed to render text for entity " << en
                << " (Bad font key: " << textComp.fontKey << "): " << e.what();
        }
    }

    /**
     * \brief Calculate and update text halfsize 
     */
    void TextRenderSystem::UpdateTextBounds(Entity entity, EntityManager& entityMgr, TextComponent& textComp, const Entity camera) {
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
        if (camera != 0 && cameraSystem_ && entityMgr.HasComponent(entity, GUITagType)) {
            textComp.halfSize = cameraSystem_->WorldToScreen(totalSize) * 0.5f;
        }
        else {
            textComp.halfSize = totalSize * 0.5f;
        }
    }

    /**
     * \brief Calculate and update text wrappedLines
     */
    void TextRenderSystem::UpdateWrappedText(TextComponent& textComp) {
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

} // namespace Framework