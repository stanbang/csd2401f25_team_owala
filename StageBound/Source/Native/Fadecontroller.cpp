/*********************************************************************//**
 * \file   FadeController.cpp
 *
 * \brief  Fullscreen fade overlay controller.
 *         Hooks into GameManager's existing fade system (FadeOut/FadeIn/
 *         TickFadeOverlay) and draws the black overlay quad via the
 *         overlayDrawCallback_. Make the entity Persistent (DDOL) so it
 *         survives all scene changes.
 *********************************************************************/
#include "pch.h"
#include "FadeController.h"
#include "ScriptRegistry.h"
#include "GameManager.h"
#include "Logger.h"

REGISTER_SCRIPT(FadeController, "FadeController")

FadeController* FadeController::s_instance = nullptr;

// -----------------------------------------------------------------------
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
void FadeController::OnUpdate() {
    // Nothing needed - GameManager drives the fade
}

// -----------------------------------------------------------------------
void FadeController::OnDestroy() {
    // Unregister callback so GameManager doesn't call a dangling lambda
    GameManager::GetInstance().SetOverlayDrawCallback(nullptr);

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
void FadeController::FadeIn(float duration, std::function<void()> onComplete) {
    GameManager::GetInstance().FadeIn(
        (duration > 0.0f) ? duration : fadeDuration, onComplete);
}

void FadeController::FadeOut(float duration, std::function<void()> onComplete) {
    GameManager::GetInstance().FadeOut(
        (duration > 0.0f) ? duration : fadeDuration, onComplete);
}

bool FadeController::IsFading() const {
    return GameManager::GetInstance().IsFading();
}

float FadeController::GetAlpha() const {
    return GameManager::GetInstance().GetFadeAlpha();
}

// -----------------------------------------------------------------------
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