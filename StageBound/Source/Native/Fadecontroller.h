#pragma once
#include "ScriptableEntity.h"
#include "Graphics.h"
#include <functional>

using namespace Framework;

class FadeController : public ScriptableEntity {
public:
    float fadeDuration = 1.0f;
    bool  fadeInOnCreate = true;

    static FadeController* s_instance;
    static FadeController* Get() { return s_instance; }

    // ---- External API ----
    void FadeIn(float duration = -1.0f, std::function<void()> onComplete = nullptr);
    void FadeOut(float duration = -1.0f, std::function<void()> onComplete = nullptr);
    bool IsFading() const;
    float GetAlpha() const;

protected:
    void OnCreate() override;
    void OnUpdate() override;
    void OnDestroy() override;

private:
    void DrawOverlay();
    GLuint m_blackTex = 0;
};