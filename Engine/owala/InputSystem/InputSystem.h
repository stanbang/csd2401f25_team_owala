/*****************************************************************//**
 * \file   InputSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 * \brief
 *   Engine input subsystem that wraps GLFW keyboard/mouse state into a
 *   frame-based API (Down/Pressed/Released), with integer and enum
 *   query overloads, cursor locking, and mouse delta tracking.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include <cstring>     // std::memcpy / std::memset

struct GLFWwindow;     // fwd-declare

namespace Framework {

    class WindowsSystem;

    /**
     * \class InputSystem
     * \brief Frame-synchronous input layer built on GLFW.
     *
     * InputSystem polls GLFW each frame and exposes simple queries:
     * - Down: current state
     * - Pressed: went from up?down this frame
     * - Released: went from down?up this frame
     *
     * It also tracks mouse position and per-frame delta, and can lock
     * (confine/hide) the cursor via GLFW input modes.
     *
     * Typical usage:
     * \code
     * InputSystem input(winSys);
     * input.Initialize();
     * // per-frame:
     * input.Update(dt);
     * if (input.KeyPressed(InputSystem::Key::Space)) Jump();
     * double mx, my; input.GetMousePos(&mx, &my);
     * \endcode
     *
     * \note Call Initialize() after the window exists (WindowsSystem created).
     * \note Call Update(dt) exactly once per frame to advance edge states.
     */
    class InputSystem : public ISystem {
    public:
        /**
         * \enum Key
         * \brief Friendly keyboard keys mapped to GLFW key codes internally.
         * \details This list is a subset/superset convenience. You can extend it
         *          without breaking the integer-based APIs.
         */
        enum class Key {
            A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
            Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
            Enter, Escape, Tab, Backspace,
            Space, PageUp, PageDown, Home, End, Insert, Delete,
            Left, Right, Up, Down, LShift, RShift, LCtrl, RCtrl, LAlt, RAlt,
            F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
        };

        /**
         * \enum MouseButton
         * \brief Mouse buttons mapped to GLFW button indices internally.
         */
        enum class MouseButton { Left, Right, Middle, Button4, Button5 };

        /// \brief Maximum sampled key/button ranges (safe superset of GLFW codes).
        static const int kMaxKeys = 512;
        static const int kMaxButtons = 8;

        /**
         * \brief Construct the input system tied to a WindowsSystem.
         * \param winSys WindowsSystem used to obtain the GLFWwindow*.
         */
        explicit InputSystem(WindowsSystem& winSys) : winSys_(&winSys) {
            std::memset(keyNow_, 0, sizeof(keyNow_));
            std::memset(keyPrev_, 0, sizeof(keyPrev_));
            std::memset(btnNow_, 0, sizeof(btnNow_));
            std::memset(btnPrev_, 0, sizeof(btnPrev_));
        }

        // ---------------- ISystem ----------------

        /**
         * \brief Initialize the input system.
         * \details Caches the GLFWwindow* from WindowsSystem; must be called after
         *          the window is created.
         */
        void Initialize() override;

        /**
         * \brief Poll input devices and update edge states.
         * \param dt Delta time in seconds (not required for polling, stored for symmetry).
         * \details Copies current ? previous state, polls GLFW for fresh state,
         *          updates mouse position and per-frame delta.
         */
        void Update(float dt) override;

        /**
         * \brief Shut down input system (no-op).
         * \details Present for interface completeness; nothing to release.
         */
        void Shutdown() override {}

        // -------- Integer (GLFW-style) queries --------
        /**
         * \brief Is the key currently down?
         * \param key GLFW key code (e.g., GLFW_KEY_W).
         * \return True if down this frame.
         */
        bool KeyDown(int key) const;

        /**
         * \brief Did the key transition up?down this frame?
         * \param key GLFW key code.
         * \return True if pressed this frame.
         */
        bool KeyPressed(int key) const;

        /**
         * \brief Did the key transition down?up this frame?
         * \param key GLFW key code.
         * \return True if released this frame.
         */
        bool KeyReleased(int key) const;

        /**
         * \brief Is the mouse button currently down?
         * \param button GLFW mouse button index (e.g., GLFW_MOUSE_BUTTON_LEFT).
         * \return True if down this frame.
         */
        bool MouseDown(int button) const;

        /**
         * \brief Was the mouse button pressed this frame?
         * \param button GLFW mouse button index.
         * \return True if pressed this frame.
         */
        bool MousePressed(int button) const;

        /**
         * \brief Was the mouse button released this frame?
         * \param button GLFW mouse button index.
         * \return True if released this frame.
         */
        bool MouseReleased(int button) const;

        // -------- Enum convenience overloads --------
        /// \copydoc KeyDown(int)
        bool KeyDown(Key k) const;
        /// \copydoc KeyPressed(int)
        bool KeyPressed(Key k) const;
        /// \copydoc KeyReleased(int)
        bool KeyReleased(Key k) const;

        /// \copydoc MouseDown(int)
        bool MouseDown(MouseButton b) const;
        /// \copydoc MousePressed(int)
        bool MousePressed(MouseButton b) const;
        /// \copydoc MouseReleased(int)
        bool MouseReleased(MouseButton b) const;

        // -------- ASCII helpers --------
        /**
         * \brief Is the ASCII key (e.g., 'W', 'd', '0') currently down?
         * \param c Printable ASCII character.
         * \return True if down this frame.
         * \note Case is mapped to corresponding GLFW virtual key where applicable.
         */
        bool KeyDown(char c) const;

        /// \copydoc KeyPressed(char)
        bool KeyPressed(char c) const;

        /// \copydoc KeyReleased(char)
        bool KeyReleased(char c) const;

        // -------- Mouse state --------
        /**
         * \brief Get current mouse cursor position in window coordinates.
         * \param x [out] Current X position.
         * \param y [out] Current Y position.
         */
        void GetMousePos(double* x, double* y) const;

        /**
         * \brief Get per-frame mouse movement (current - previous).
         * \param dx [out] Delta X since last Update().
         * \param dy [out] Delta Y since last Update().
         */
        void GetMouseDelta(double* dx, double* dy) const;

        /**
         * \brief Enable or disable cursor lock (capture/hide).
         * \param locked If true, lock and hide the cursor; else, restore normal cursor.
         * \details Uses GLFW input modes to confine/hide the cursor for mouselook.
         */
        void SetCursorLocked(bool locked);


        /**
         * \brief Enable or disable keyboard lock (capture/hide).
         * \param locked If true, lock and disable keyboard input; else, restore keyboard input.
         * \details Uses GLFW input modes to confine/hide the cursor for mouselook.
         */
        void SetKeyboardLocked(bool locked);

    private:
        // ---- Mapping helpers (implemented in .cpp) ----
        /**
         * \brief Map a Key enum to a GLFW key code.
         * \param k Key enum.
         * \return GLFW key code (integer).
         */
        static int ToGlfwKey(Key k);

        /**
         * \brief Map an ASCII character to a GLFW key code.
         * \param c Printable ASCII.
         * \return GLFW key code (integer).
         */
        static int ToGlfwKeyFromChar(char c);

        /**
         * \brief Map a MouseButton enum to a GLFW mouse button index.
         * \param b MouseButton enum.
         * \return GLFW mouse button index (integer).
         */
        static int ToGlfwMouse(MouseButton b);

        // ---- Backing state ----
        WindowsSystem* winSys_ = nullptr;  //!< Non-owning; used to fetch GLFWwindow*.
        GLFWwindow* window_ = nullptr;  //!< Set in Initialize().

        bool   keyNow_[kMaxKeys],    //!< Current frame key states.
            keyPrev_[kMaxKeys];   //!< Previous frame key states.
        bool   btnNow_[kMaxButtons], //!< Current frame mouse button states.
            btnPrev_[kMaxButtons];//!< Previous frame mouse button states.

        double mouseX_ = 0.0, mouseY_ = 0.0;           //!< Current cursor position.
        double prevMouseX_ = 0.0, prevMouseY_ = 0.0;   //!< Previous cursor position.
        bool   cursorLocked_ = false;                  //!< Cursor lock flag.
        bool   keyboardLocked_ = false;                //!< Keyboard lock flag.
    };

} // namespace Framework
