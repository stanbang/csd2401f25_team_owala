#include "InputSystem.h"
#include "WindowsSystem.h"
#include <GLFW/glfw3.h>
#include <cctype>      // std::toupper

namespace Framework {

    // ---------------------- ISystem ----------------------

    void InputSystem::Initialize() {
        // Get the actual GLFWwindow* AFTER WindowsSystem created it
        window_ = winSys_->window();
        if (!window_) return;  // guard: do nothing if window creation failed

        // Seed mouse position
        glfwGetCursorPos(window_, &mouseX_, &mouseY_);
        prevMouseX_ = mouseX_;
        prevMouseY_ = mouseY_;
    }

    void InputSystem::Update(float /*dt*/) {
        if (!window_) return;

        // Ensure events are pumped (ok to call here even if another system calls it too)
        glfwPollEvents();

        // Roll current -> previous
        std::memcpy(keyPrev_, keyNow_, sizeof(keyNow_));
        std::memcpy(btnPrev_, btnNow_, sizeof(btnNow_));

        // Sample a safe range of key codes
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

    // ---------------------- integer queries ----------------------

    bool InputSystem::KeyDown(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return keyNow_[key];
    }
    bool InputSystem::KeyPressed(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return keyNow_[key] && !keyPrev_[key];
    }
    bool InputSystem::KeyReleased(int key) const {
        if (key < 0 || key >= kMaxKeys || keyboardLocked_) return false;
        return !keyNow_[key] && keyPrev_[key];
    }

    bool InputSystem::MouseDown(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return btnNow_[b];
    }
    bool InputSystem::MousePressed(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return btnNow_[b] && !btnPrev_[b];
    }
    bool InputSystem::MouseReleased(int b) const {
        if (b < 0 || b >= kMaxButtons) return false;
        return !btnNow_[b] && btnPrev_[b];
    }

    // ---------------------- enum queries ----------------------

    bool InputSystem::KeyDown(Key k) const { return KeyDown(ToGlfwKey(k)); }
    bool InputSystem::KeyPressed(Key k) const { return KeyPressed(ToGlfwKey(k)); }
    bool InputSystem::KeyReleased(Key k) const { return KeyReleased(ToGlfwKey(k)); }

    bool InputSystem::MouseDown(MouseButton b) const { return MouseDown(ToGlfwMouse(b)); }
    bool InputSystem::MousePressed(MouseButton b) const { return MousePressed(ToGlfwMouse(b)); }
    bool InputSystem::MouseReleased(MouseButton b) const { return MouseReleased(ToGlfwMouse(b)); }

    // ---------------------- ASCII helpers ----------------------

    bool InputSystem::KeyDown(char c) const { return KeyDown(ToGlfwKeyFromChar(c)); }
    bool InputSystem::KeyPressed(char c) const { return KeyPressed(ToGlfwKeyFromChar(c)); }
    bool InputSystem::KeyReleased(char c) const { return KeyReleased(ToGlfwKeyFromChar(c)); }

    // ---------------------- mouse & cursor ----------------------

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
        if (!window_) return;
        glfwSetInputMode(window_, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void InputSystem::SetKeyboardLocked(bool locked) {
        keyboardLocked_ = locked;
    }

    // ---------------------- mappings ----------------------

    static int alphaToGlfw(int upper) { return GLFW_KEY_A + (upper - 'A'); }
    static int digitToGlfw(int digit) { return GLFW_KEY_0 + digit; }

    int InputSystem::ToGlfwKey(Key k) {
        switch (k) {
        case Key::A: return GLFW_KEY_A; case Key::B: return GLFW_KEY_B; case Key::C: return GLFW_KEY_C;
        case Key::D: return GLFW_KEY_D; case Key::E: return GLFW_KEY_E; case Key::F: return GLFW_KEY_F;
        case Key::G: return GLFW_KEY_G; case Key::H: return GLFW_KEY_H; case Key::I: return GLFW_KEY_I;
        case Key::J: return GLFW_KEY_J; case Key::K: return GLFW_KEY_K; case Key::L: return GLFW_KEY_L;
        case Key::M: return GLFW_KEY_M; case Key::N: return GLFW_KEY_N; case Key::O: return GLFW_KEY_O;
        case Key::P: return GLFW_KEY_P; case Key::Q: return GLFW_KEY_Q; case Key::R: return GLFW_KEY_R;
        case Key::S: return GLFW_KEY_S; case Key::T: return GLFW_KEY_T; case Key::U: return GLFW_KEY_U;
        case Key::V: return GLFW_KEY_V; case Key::W: return GLFW_KEY_W; case Key::X: return GLFW_KEY_X;
        case Key::Y: return GLFW_KEY_Y; case Key::Z: return GLFW_KEY_Z;
        case Key::Num0: return GLFW_KEY_0; case Key::Num1: return GLFW_KEY_1; case Key::Num2: return GLFW_KEY_2;
        case Key::Num3: return GLFW_KEY_3; case Key::Num4: return GLFW_KEY_4; case Key::Num5: return GLFW_KEY_5;
        case Key::Num6: return GLFW_KEY_6; case Key::Num7: return GLFW_KEY_7; case Key::Num8: return GLFW_KEY_8;
        case Key::Num9: return GLFW_KEY_9;
        case Key::Enter: return GLFW_KEY_ENTER; case Key::Escape: return GLFW_KEY_ESCAPE; 
        case Key::Tab: return GLFW_KEY_TAB; case Key::Backspace: return GLFW_KEY_BACKSPACE;
        case Key::Space: return GLFW_KEY_SPACE;
		case Key::PageUp: return GLFW_KEY_PAGE_UP; case Key::PageDown: return GLFW_KEY_PAGE_DOWN;
		case Key::Home: return GLFW_KEY_HOME; case Key::End: return GLFW_KEY_END;
        case Key::Insert: return GLFW_KEY_INSERT; case Key::Delete: return GLFW_KEY_DELETE;
        case Key::Left: return GLFW_KEY_LEFT; case Key::Right: return GLFW_KEY_RIGHT;
        case Key::Up: return GLFW_KEY_UP; case Key::Down: return GLFW_KEY_DOWN;
        case Key::LShift: return GLFW_KEY_LEFT_SHIFT; case Key::RShift: return GLFW_KEY_RIGHT_SHIFT;
        case Key::LCtrl:  return GLFW_KEY_LEFT_CONTROL; case Key::RCtrl:  return GLFW_KEY_RIGHT_CONTROL;
        case Key::LAlt:   return GLFW_KEY_LEFT_ALT;     case Key::RAlt:   return GLFW_KEY_RIGHT_ALT;
        case Key::F1: return GLFW_KEY_F1; case Key::F2: return GLFW_KEY_F2; case Key::F3: return GLFW_KEY_F3;
        case Key::F4: return GLFW_KEY_F4; case Key::F5: return GLFW_KEY_F5; case Key::F6: return GLFW_KEY_F6;
        case Key::F7: return GLFW_KEY_F7; case Key::F8: return GLFW_KEY_F8; case Key::F9: return GLFW_KEY_F9;
        case Key::F10: return GLFW_KEY_F10; case Key::F11: return GLFW_KEY_F11; case Key::F12: return GLFW_KEY_F12;
        }
        return GLFW_KEY_UNKNOWN;
    }

    int InputSystem::ToGlfwKeyFromChar(char c) {
        if (c >= '0' && c <= '9') return digitToGlfw(c - '0');
        int u = std::toupper(static_cast<unsigned char>(c));
        if (u >= 'A' && u <= 'Z') return alphaToGlfw(u);
        return GLFW_KEY_UNKNOWN;
    }

    int InputSystem::ToGlfwMouse(MouseButton b) {
        switch (b) {
        case MouseButton::Left:   return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right:  return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::Button4:return GLFW_MOUSE_BUTTON_4;
        case MouseButton::Button5:return GLFW_MOUSE_BUTTON_5;
        }
        return GLFW_MOUSE_BUTTON_LEFT;
    }

} // namespace Framework
