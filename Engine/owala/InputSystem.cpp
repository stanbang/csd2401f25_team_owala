#include "InputSystem.h"
#include "WindowsSystem.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <cstdio>

namespace Framework {

    InputSystem::InputSystem(WindowsSystem& win)
        : win_(&win) {
        // Start with clean state
        std::memset(keyNow_, 0, sizeof(keyNow_));
        std::memset(keyPrev_, 0, sizeof(keyPrev_));
        std::memset(btnNow_, 0, sizeof(btnNow_));
        std::memset(btnPrev_, 0, sizeof(btnPrev_));
    }

    void InputSystem::Initialize() {
        // Fetch the real GLFWwindow* now that WindowsSystem has created it
        window_ = win_ ? win_->window() : nullptr;
        if (!window_) {
            std::fprintf(stderr, "InputSystem::Initialize – window is null\n");
            return; // keep running; Update() guards against null
        }
        glfwGetCursorPos(window_, &mouseX_, &mouseY_);
        prevMouseX_ = mouseX_;
        prevMouseY_ = mouseY_;
    }

    void InputSystem::Update(float /*dt*/) {
        if (!window_) return; // safety

        // Save previous states
        std::memcpy(keyPrev_, keyNow_, sizeof(keyNow_));
        std::memcpy(btnPrev_, btnNow_, sizeof(btnNow_));

        // Keyboard
        for (int i = 0; i < kMaxKeys; ++i) {
            int s = glfwGetKey(window_, i);
            keyNow_[i] = (s == GLFW_PRESS) || (s == GLFW_REPEAT);
        }
        // Mouse buttons
        for (int b = 0; b < kMaxButtons; ++b) {
            int s = glfwGetMouseButton(window_, b);
            btnNow_[b] = (s == GLFW_PRESS);
        }

        // Mouse position / delta
        prevMouseX_ = mouseX_;
        prevMouseY_ = mouseY_;
        glfwGetCursorPos(window_, &mouseX_, &mouseY_);
    }

    bool InputSystem::KeyDown(int key) const {
        return (key >= 0 && key < kMaxKeys) ? keyNow_[key] : false;
    }
    bool InputSystem::KeyPressed(int key) const {
        return (key >= 0 && key < kMaxKeys) ? (keyNow_[key] && !keyPrev_[key]) : false;
    }
    bool InputSystem::KeyReleased(int key) const {
        return (key >= 0 && key < kMaxKeys) ? (!keyNow_[key] && keyPrev_[key]) : false;
    }

    bool InputSystem::MouseDown(int button) const {
        return (button >= 0 && button < kMaxButtons) ? btnNow_[button] : false;
    }
    bool InputSystem::MousePressed(int button) const {
        return (button >= 0 && button < kMaxButtons) ? (btnNow_[button] && !btnPrev_[button]) : false;
    }
    bool InputSystem::MouseReleased(int button) const {
        return (button >= 0 && button < kMaxButtons) ? (!btnNow_[button] && btnPrev_[button]) : false;
    }

    void InputSystem::GetMousePos(double* x, double* y) const {
        if (x) *x = mouseX_;
        if (y) *y = mouseY_;
    }
    void InputSystem::GetMouseDelta(double* dx, double* dy) const {
        if (dx) *dx = mouseX_ - prevMouseX_;
        if (dy) *dy = mouseY_ - prevMouseY_;
    }

    void InputSystem::SetCursorLocked(bool locked) {
        cursorLocked_ = locked;
        if (window_) {
            glfwSetInputMode(window_, GLFW_CURSOR,
                locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }
    }

} // namespace Framework
