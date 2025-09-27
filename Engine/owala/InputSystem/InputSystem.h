/*****************************************************************//**
 * \file   InputSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 *
 * \brief
 *
 * \copyright Copyright (C) 2025 DigiPen Institute of Technology.
 * Reproduction or disclosure of this file or its contents without the prior
 * written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include <cstring>     // std::memcpy / std::memset

struct GLFWwindow;     // fwd-declare

namespace Framework {

    class WindowsSystem;

    class InputSystem : public ISystem {
    public:
        // Friendly key/mouse enums (you can extend as you like)
        enum class Key {
            A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
            Space, Enter, Escape, Tab, Backspace,
            Left, Right, Up, Down, LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
        };
        enum class MouseButton { Left, Right, Middle, Button4, Button5 };

        // We sample a fixed range of codes (safe superset of GLFW’s)
        static const int kMaxKeys = 512;
        static const int kMaxButtons = 8;

        explicit InputSystem(WindowsSystem& winSys) : winSys_(&winSys) {
            std::memset(keyNow_, 0, sizeof(keyNow_));
            std::memset(keyPrev_, 0, sizeof(keyPrev_));
            std::memset(btnNow_, 0, sizeof(btnNow_));
            std::memset(btnPrev_, 0, sizeof(btnPrev_));
        }

        // ISystem
        void Initialize() override;        // grabs GLFWwindow* here (after the window exists)
        void Update(float dt) override;    // polls keys, buttons, cursor
        void Shutdown() override {}        // nothing to do

        // Integer-based (GLFW-style) queries – works with GLFW_KEY_* constants
        bool KeyDown(int key) const;
        bool KeyPressed(int key) const;
        bool KeyReleased(int key) const;

        bool MouseDown(int button) const;
        bool MousePressed(int button) const;
        bool MouseReleased(int button) const;

        // Enum-based convenience
        bool KeyDown(Key k) const;
        bool KeyPressed(Key k) const;
        bool KeyReleased(Key k) const;

        bool MouseDown(MouseButton b) const;
        bool MousePressed(MouseButton b) const;
        bool MouseReleased(MouseButton b) const;

        // ASCII helpers (e.g., 'W', 'd', '0')
        bool KeyDown(char c) const;
        bool KeyPressed(char c) const;
        bool KeyReleased(char c) const;

        // Mouse state
        void GetMousePos(double* x, double* y) const;
        void GetMouseDelta(double* dx, double* dy) const;
        void SetCursorLocked(bool locked);

    private:
        // Mappings (implemented in .cpp)
        static int ToGlfwKey(Key k);
        static int ToGlfwKeyFromChar(char c);
        static int ToGlfwMouse(MouseButton b);

        WindowsSystem* winSys_ = nullptr;  // we fetch the real GLFWwindow* from here
        GLFWwindow* window_ = nullptr;  // set in Initialize()

        bool   keyNow_[kMaxKeys], keyPrev_[kMaxKeys];
        bool   btnNow_[kMaxButtons], btnPrev_[kMaxButtons];
        double mouseX_ = 0.0, mouseY_ = 0.0;
        double prevMouseX_ = 0.0, prevMouseY_ = 0.0;
        bool   cursorLocked_ = false;
    };

} // namespace Framework
