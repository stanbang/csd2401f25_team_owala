/*********************************************************************//**
 * \file   FadeController.h
 *
 * \primary_author
 *      Jordain Ng Ying Zhe (100%)
 *
 * \brief  Fullscreen fade overlay controller script.
 *         Provides a configurable black fade overlay that works across
 *         scene transitions by making the entity persistent (DDOL).
 *
 * \details
 *      The FadeController is a script component that integrates with
 *      GameManager's existing fade system (FadeOut/FadeIn/TickFadeOverlay).
 *      It creates a fullscreen black quad with adjustable alpha and draws
 *      it via the overlayDrawCallback_ hook in GameManager. The entity
 *      should be marked as persistent (DDOL) so it survives all scene
 *      changes and continues to fade during loading transitions.
 *
 *      Key features:
 *        - Automatic fullscreen black overlay quad
 *        - Configurable fade-in on creation (fadeInOnCreate)
 *        - Public API for triggering fades (FadeIn/FadeOut)
 *        - Singleton instance access for easy lookup
 *        - Persistent across scene changes when properly configured
 *
 *      Example Usage:
 *      \code
 *      // In a persistent DDOL scene, create entity with this script
 *      Entity fadeEntity = ecs.CreateEntity();
 *      fadeEntity.AddComponent<Transform>();
 *
 *      auto& script = fadeEntity.AddComponent<ScriptComponent>();
 *      script.Bind<FadeController>();
 *
 *      // Configure (optional - defaults are usually fine)
 *      auto* controller = fadeEntity.GetComponent<ScriptComponent>()
 *          .GetScriptInstance<FadeController>();
 *      controller->fadeInOnCreate = true;  // Fade in when level starts
 *      controller->fadeDuration = 0.5f;     // 0.5 second fades
 *
 *      // Later, trigger a fade out before loading next level
 *      FadeController::Get()->FadeOut(0.3f, []() {
 *          // Fade complete - load next scene
 *          SceneManager::GetInstance().LoadScene("Level2");
 *      });
 *      \endcode
 *
 * \note
 *      This script must be attached to an entity in a persistent DDOL
 *      (Don't Destroy On Load) scene to survive scene transitions.
 *      The entity should have a Transform component (position is ignored).
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without
 *      the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ScriptableEntity.h"
#include "Graphics.h"
#include <functional>

using namespace Framework;

/**
 * @brief Script that provides a fullscreen black fade overlay for scene transitions.
 *
 * This script creates and manages a fullscreen black quad that can fade in/out
 * during scene transitions. It integrates with GameManager's fade system and
 * should be attached to a persistent entity (DDOL) to survive scene changes.
 *
 * The controller maintains a singleton instance (s_instance) that can be accessed
 * statically from anywhere in the codebase via Get().
 */
class FadeController : public ScriptableEntity {
public:
    /**
     * @brief Duration of fade animations in seconds.
     *
     * Default: 1.0f
     * This value is used when no explicit duration is passed to FadeIn/FadeOut.
     */
    float fadeDuration = 1.0f;

    /**
     * @brief Whether to automatically fade in when the controller is created.
     *
     * If true, calls FadeIn(fadeDuration) automatically in OnCreate.
     * Default: true
     */
    bool  fadeInOnCreate = true;

    /**
     * @brief Singleton instance pointer for global access.
     *
     * Set in OnCreate, cleared in OnDestroy.
     */
    static FadeController* s_instance;

    /**
     * @brief Gets the singleton instance of FadeController.
     *
     * @return Pointer to the active FadeController instance, or nullptr if none exists
     */
    static FadeController* Get() { return s_instance; }

    // -------- Public Fade Control API --------
    /**
     * @brief Triggers a fade-in (black to screen).
     *
     * @param duration Optional custom duration. If <= 0, uses fadeDuration.
     * @param onComplete Callback to execute when fade completes
     */
    void FadeIn(float duration = -1.0f, std::function<void()> onComplete = nullptr);

    /**
     * @brief Triggers a fade-out (screen to black).
     *
     * @param duration Optional custom duration. If <= 0, uses fadeDuration.
     * @param onComplete Callback to execute when fade completes
     */
    void FadeOut(float duration = -1.0f, std::function<void()> onComplete = nullptr);

    /**
     * @brief Checks if a fade animation is currently active.
     *
     * @return true if fading, false otherwise
     */
    bool IsFading() const;

    /**
     * @brief Gets the current fade alpha value.
     *
     * @return Current alpha (0.0f = transparent, 1.0f = fully black)
     */
    float GetAlpha() const;

protected:
    /**
     * @brief Initializes the fade controller when the entity is created.
     *
     * Creates a 1x1 black texture for the overlay quad and registers the
     * draw callback with GameManager. If fadeInOnCreate is true, triggers
     * an automatic fade-in.
     */
    void OnCreate() override;

    /**
     * @brief Updates the fade controller each frame.
     *
     * Empty implementation - the fade is driven by GameManager's TickFadeOverlay.
     */
    void OnUpdate() override;

    /**
     * @brief Cleans up resources when the entity is destroyed.
     *
     * Unregisters the draw callback and deletes the black texture.
     * Clears the singleton instance if this is the active one.
     */
    void OnDestroy() override;

private:
    /**
     * @brief Draws the fade overlay quad.
     *
     * Called by GameManager via overlayDrawCallback_ each frame.
     * Creates a fullscreen black quad with current fade alpha.
     * Does nothing if alpha <= 0 or if no black texture exists.
     */
    void DrawOverlay();

    GLuint m_blackTex = 0; //!< OpenGL texture handle for a single black pixel (used for overlay quad)
};