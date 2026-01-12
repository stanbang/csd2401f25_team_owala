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
            OPENING     // Animation playing: Closed -> Open
        };

        static void PlayAllClose(std::function<void()> onComplete = nullptr);
        static void PlayAllOpen();

        void PlayClose(std::function<void()> onComplete = nullptr);
        void PlayOpen(std::function<void()> onComplete = nullptr);

        // Helper to force closed state (e.g., if starting directly in loading screen)
        void SetClosed();

        // Destructor
        virtual ~TransitionController();

        // Static Reset function
        static void Reset();

    protected:
        void OnCreate() override;
        void OnUpdate() override;
        void OnDestroy() override;

    private:
        static std::vector<TransitionController*> allControllers;

        TransitionState state_ = TransitionState::HIDDEN;
        std::function<void()> onTransitionComplete_ = nullptr;

        // Animation config
        float animTimer_ = 0.0f;
        float animDuration_ = 1.0f; // Time to open or close
    };
}