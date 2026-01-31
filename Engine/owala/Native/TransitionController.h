/*****************************************************************//**
 * \file   TransitionController.h
 *
 * \primary_author
 *      Ng Jing Wen
 * \secondary_author
 *
 *
 * \brief	Declarations for the TransitionController script to manage
 *          curtain open/close transitions between scenes.
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ScriptableEntity.h"
#include <functional>
#include <vector>

namespace Framework {

    class TransitionController : public ScriptableEntity {
    public:
        enum class TransitionState {
            HIDDEN,     // Curtains open, game visible
            CLOSING,    // Animation playing: Open -> Closed
            CLOSED,     // Curtains closed, screen covered
            OPENING,     // Animation playing: Closed -> Open
			WAITING	// Waiting state (no animation)
        };

        static void PlayAllClose(std::function<void()> onComplete = nullptr);
        static void PlayAllOpen();

        void PlayClose(std::function<void()> onComplete = nullptr);
        void PlayOpen(std::function<void()> onComplete = nullptr);
        static void ForceHideAll();
        void ForceHide();

        // Helper to force closed state (e.g., if starting directly in loading screen)
        void SetClosed();

        // Destructor
        virtual ~TransitionController();

        // Static Reset function
        static void Reset();

		// If true, means the scene will start with curtains closed
        static bool s_startNextSceneClosed;

    protected:
        void OnCreate() override;
        void OnUpdate() override;
        void OnDestroy() override;

    private:
        static std::vector<TransitionController*> allControllers;
        static float s_lastOpenEventTime;

        TransitionState state_ = TransitionState::HIDDEN;
        std::function<void()> onTransitionComplete_ = nullptr;

        // Animation config
        float animTimer_ = 0.0f;
        float animDuration_ = 1.0f; // Time to open or close
        float waitTimer_ = 0.0f;      // timer for the delay
        const float WAIT_DURATION = 1.0f; // 1 second delay

        void CheckStartClosed();
        void StartWaitingState();
    };
}