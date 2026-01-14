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
        CameraSystem* cameraSystem_ = GameManager::GetInstance().GetSystems().camera_;

        // Retrieve active camera
        Entity activeCamera = 0;
        if (cameraSystem_) {
            activeCamera = cameraSystem_->GetActiveCamera();
        }

        //float left = -1.0f, right = 1.0f, bottom = -1.0f, top = 1.0f;
        //if (activeCamera != 0 && cameraSystem_) {
        //    cameraSystem_->GetViewBounds(activeCamera, left, right, bottom, top);
        //}

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

            RenderText(entity, entityMgr, textComp, activeCamera);
        }
    }

    /**
     * \brief Renders all visible text components.
     */
    void TextRenderSystem::RenderText(const Entity en, EntityManager& entityMgr, const TextComponent& textComp, const Entity camera) {
        GameManager& gm = GameManager::GetInstance();
        CameraSystem* cameraSystem_ = gm.GetSystems().camera_;
        ResourceManager& rcm_ = *gm.GetSystems().resourceManager_;
        Graphics& gfx_ = *gm.GetSystems().gfx_;

        try {
            Font& font = rcm_.get<Font>(textComp.fontKey);
            const Vector2D& textPos = entityMgr.GetComponent<Transform>(en).position;
            bool isUIElement = entityMgr.HasComponent(en, GUITagType);

            float screenW = static_cast<float>(gfx_.getScreenWidth());
            float screenH = static_cast<float>(gfx_.getScreenHeight());
            
            float zoom = 1.0f;
            Vector2D camPos = { 0.0f, 0.0f };
            Vector2D camSize = { screenW, screenH };

            if (camera != 0 && cameraSystem_ && !isUIElement) {
                const Camera& cam = cameraSystem_->GetCamera(camera);
                //zoom = cam.zoom;
                camPos = cam.position;
                camSize = cam.size;
            }

            Vector2D screenTextPos = textPos;

            // Texts without GUI Tag need to convert to screen space
            if (!isUIElement) {
                // Subtract Camera Position from Text Position
                float relativeX = textPos.x - camPos.x;
                float relativeY = textPos.y - camPos.y;

                // offset so that pos is the middle of the text
                relativeX -= textComp.halfSize.x;
                relativeY += textComp.halfSize.y;

                // Convert Relative Position to NDC
                screenTextPos = Vector2D(
                    (relativeX / camSize.x) * 2.0f / zoom,
                    (relativeY / camSize.y) * 2.0f / zoom
                );
            }
            else {
                // Texts with GUI tag already use screen coords
                // offset so that pos is the middle of the text
                screenTextPos.x -= textComp.halfSize.x;
                screenTextPos.y += textComp.halfSize.y;
            }

            // get line height for offsetting the y when rendering multiple lines of text
            const float line_height_px = font.GetLineHeight(textComp.scale);
            float line_height_ndc = (line_height_px / screenH) * 2.0f;

            // draw each line of text
            for (const std::string& line : textComp.wrappedLines) {
                gfx_.drawText(font, line, screenTextPos, textComp.scale / zoom, textComp.color);
                screenTextPos.y -= line_height_ndc / zoom;
            }
        }
        catch (const std::exception& e) {
            Logger::Get().Error(DebugType::TextRenderSystem)
                << "Failed to render text for entity " << en
                << " (Bad font key: " << textComp.fontKey << "): " << e.what() << std::endl;
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