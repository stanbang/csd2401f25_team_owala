#pragma once
#include "ISystem.h"

// fwd decls to keep the header light
struct GLFWwindow;
namespace Framework { class WindowsSystem; }

namespace Framework {

    class InputSystem : public ISystem {
    public:
        explicit InputSystem(WindowsSystem& win);

        // ISystem
        void Initialize() override;   // grab the GLFWwindow* here (window exists now)
        void Update(float dt) override;
        void Shutdown() override {}

        // Keyboard queries (GLFW key codes)
        bool KeyDown(int key) const;
        bool KeyPressed(int key) const;
        bool KeyReleased(int key) const;

        // Mouse queries (GLFW mouse button codes)
        bool MouseDown(int button) const;
        bool MousePressed(int button) const;
        bool MouseReleased(int button) const;

        // Mouse position (pixels) and delta since last frame
        void GetMousePos(double* x, double* y) const;
        void GetMouseDelta(double* dx, double* dy) const;

        // Lock/unlock mouse cursor (useful for FPS controls)
        void SetCursorLocked(bool locked);

    private:
        static constexpr int kMaxKeys = 512;
        static constexpr int kMaxButtons = 8;

        WindowsSystem* win_ = nullptr;   // we fetch the GLFWwindow* from this
        GLFWwindow* window_ = nullptr;  // set in Initialize()

        bool   keyNow_[kMaxKeys] = { false };
        bool   keyPrev_[kMaxKeys] = { false };
        bool   btnNow_[kMaxButtons] = { false };
        bool   btnPrev_[kMaxButtons] = { false };

        double mouseX_ = 0.0, mouseY_ = 0.0;
        double prevMouseX_ = 0.0, prevMouseY_ = 0.0;
        bool   cursorLocked_ = false;
    };

} // namespace Framework
