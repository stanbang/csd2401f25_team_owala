/*********************************************************************//**
 * \file   FadeController.cpp
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Fullscreen fade overlay controller implementation.
 *         Hooks into GameManager's existing fade system (FadeOut/FadeIn/
 *         TickFadeOverlay) and draws the black overlay quad via the
 *         overlayDrawCallback_. Make the entity Persistent (DDOL) so it
 *         survives all scene changes.
 *
 * \details
 *      This file contains the implementation of the FadeController script.
 *      It creates a 1x1 black texture for the overlay quad and registers
 *      a draw callback with GameManager. The controller maintains a
 *      singleton instance for easy global access and provides a clean
 *      API for triggering fade effects from anywhere in the codebase.
 *
 *      The fade values are driven entirely by GameManager's fade system;
 *      this script simply visualizes them with a fullscreen black quad.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "pch.h"
#include "FadeController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(FadeController, "FadeController")

// Initialize static singleton instance pointer
FadeController* FadeController::s_instance = nullptr;

// -----------------------------------------------------------------------

/**
 * @brief Initializes the fade controller when the entity is created.
 *
 * Performs the following setup:
 * 1. Stores itself as the singleton instance
 * 2. Creates a 1x1 black texture for the overlay quad
 * 3. Registers DrawOverlay() as the GameManager overlay callback
 * 4. Logs creation with debug info
 * 5. Triggers auto fade-in if fadeInOnCreate is true
 *
 * @note The overlay quad uses a single black pixel texture stretched
 *       across the entire screen for maximum performance.
 */
void FadeController::OnCreate() {
    s_instance = this;

    // Create a 1x1 black texture used as the fullscreen overlay quad
    glGenTextures(1, &m_blackTex);
    glBindTexture(GL_TEXTURE_2D, m_blackTex);
    const unsigned char black[4] = { 0, 0, 0, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Hook our draw function into GameManager's overlay callback.
    GameManager::GetInstance().SetOverlayDrawCallback([this]() {
        DrawOverlay();
        });

    Logger::Get().Info(DebugType::GameManager)
        << "[FadeController] Created, callback registered, blackTex="
        << m_blackTex << std::endl;

    // Auto fade-in if enabled
    if (fadeInOnCreate) {
        GameManager::GetInstance().FadeIn(fadeDuration);
    }
}

// -----------------------------------------------------------------------

/**
 * @brief Updates the fade controller each frame.
 *
 * Empty implementation because the fade animation is driven entirely
 * by GameManager's TickFadeOverlay() method.
 */
void FadeController::OnUpdate() {
    // Nothing needed - GameManager drives the fade
}

// -----------------------------------------------------------------------

/**
 * @brief Cleans up resources when the entity is destroyed.
 *
 * Performs cleanup:
 * 1. Unregisters the overlay callback from GameManager
 * 2. Deletes the OpenGL black texture
 * 3. Clears the singleton instance if this is the active one
 * 4. Logs destruction for debugging
 */
void FadeController::OnDestroy() {
    if (GameManager::Exists()) {
        GameManager::GetInstance().SetOverlayDrawCallback(nullptr);
    }

    if (m_blackTex != 0) {
        glDeleteTextures(1, &m_blackTex);
        m_blackTex = 0;
    }
    if (s_instance == this) {
        s_instance = nullptr;
    }
    Logger::Get().Info(DebugType::GameManager)
        << "[FadeController] Destroyed." << std::endl;
}

// -----------------------------------------------------------------------

/**
 * @brief Triggers a fade-in (black to screen) via GameManager.
 *
 * @param duration Optional custom duration. If <= 0, uses fadeDuration.
 * @param onComplete Callback to execute when fade completes
 */
void FadeController::FadeIn(float duration, std::function<void()> onComplete) {
    GameManager::GetInstance().FadeIn(
        (duration > 0.0f) ? duration : fadeDuration, onComplete);
}

/**
 * @brief Triggers a fade-out (screen to black) via GameManager.
 *
 * @param duration Optional custom duration. If <= 0, uses fadeDuration.
 * @param onComplete Callback to execute when fade completes
 */
void FadeController::FadeOut(float duration, std::function<void()> onComplete) {
    GameManager::GetInstance().FadeOut(
        (duration > 0.0f) ? duration : fadeDuration, onComplete);
}

/**
 * @brief Checks if a fade animation is currently active.
 *
 * @return true if fading, false otherwise
 */
bool FadeController::IsFading() const {
    return GameManager::GetInstance().IsFading();
}

/**
 * @brief Gets the current fade alpha value from GameManager.
 *
 * @return Current alpha (0.0f = transparent, 1.0f = fully black)
 */
float FadeController::GetAlpha() const {
    return GameManager::GetInstance().GetFadeAlpha();
}

// -----------------------------------------------------------------------

/**
 * @brief Draws the fullscreen black overlay quad.
 *
 * Called by GameManager each frame via the overlay callback.
 * Creates a fullscreen quad with the current fade alpha and draws it
 * using the batched sprite renderer.
 *
 * The quad is sized to cover the entire screen regardless of resolution
 * by using screen dimensions from Graphics system.
 *
 * @note Does nothing if alpha <= 0 or if black texture is missing.
 */
void FadeController::DrawOverlay() {
    float alpha = GameManager::GetInstance().GetFadeAlpha();
    if (alpha <= 0.0f) return;
    if (m_blackTex == 0) return;

    Graphics* gfx = GameManager::GetInstance().GetSystems().gfx_;
    if (!gfx) return;

    float w = (float)gfx->getScreenWidth();
    float h = (float)gfx->getScreenHeight();

    BatchedSprite overlay;
    overlay.topLeft = { -w, -h };
    overlay.size = { w * 2.0f, h * 2.0f };
    overlay.minUV = { 0.0f, 0.0f };
    overlay.maxUV = { 1.0f, 1.0f };
    overlay.rotation = 0.0f;
    overlay.color = { 0.0f, 0.0f, 0.0f, alpha };

    std::vector<BatchedSprite> batch = { overlay };
    gfx->drawSpriteBatch(m_blackTex, batch);
}