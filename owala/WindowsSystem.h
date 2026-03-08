/*****************************************************************//**
 * \file   WindowsSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (40%)
 * \secondary_author
 *		Yar Xuan (50%)
 *		Ng Jing Wen (10%)
 *
 * \brief
 *   Window/context management system. Wraps GLFW window creation and
 *   lifecycle, integrates with CoreEngine, and provides access to
 *   the underlying GLFWwindow pointer for other subsystems.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/
#pragma once
#include "ISystem.h"
#include "Vector2D.h"
struct GLFWwindow;

namespace Framework {
    class CoreEngine;

    /**
     * \class WindowsSystem
     * \brief Subsystem responsible for creating and managing the application window.
     *
     * WindowsSystem uses GLFW to create a window and OpenGL context. It also
     * polls events (keyboard, mouse, OS window events) during Update() and
     * cleans up resources during Shutdown().
     *
     * Other subsystems (e.g., InputSystem, Graphics) use the underlying
     * GLFWwindow* pointer provided by this class.
     *
     * Example usage:
     * \code
     * CoreEngine core;
     * WindowsSystem* winSys = new WindowsSystem(core, "My Game", 1280, 720);
     * core.AddSystem(winSys);
     * core.Initialize();
     * core.GameLoop();
     * core.DestroySystems();
     * \endcode
     */
    class WindowsSystem : public ISystem {
    public:
        /**
         * \brief Construct a WindowsSystem with window title and dimensions.
         * \param core  Reference to the CoreEngine that owns this system.
         * \param title Window title string.
         * \param w     Initial window width in pixels.
         * \param h     Initial window height in pixels.
         */
        WindowsSystem(CoreEngine& core, const char* title, int w, int h, bool fullScreen)
            : core_(&core), title_(title), windowedW_(w), windowedH_(h), fullScreen_(fullScreen) {
        }

        /**
         * \brief Initialize the window system.
         * \details Creates a GLFW window and OpenGL context. Must be called
         *          before other graphics/input systems use the window.
         */
        void Initialize() override;

        /**
         * \brief Per-frame update.
         * \param dt Delta time in seconds (unused here).
         * \details Polls GLFW events (keyboard, mouse, OS window messages).
         */
        void Update() override;

        /**
         * \brief Shut down the window system.
         * \details Destroys the GLFW window and releases associated resources.
         */
        void Shutdown() override;

        // Callback to be called by GLFW
        static void WindowFocusCallback(GLFWwindow* window, int focused);

        bool IsFocused() const { return isFocused_; }

#ifndef BUILD_EDITOR
        /**
         * \brief Toggle between full screen and windowed mode.
         */
        void ToggleFullScreen();
#endif
        /**
         * \brief Accessor for the raw GLFW window pointer.
         * \return Pointer to the underlying GLFWwindow.
         */
        GLFWwindow* window() const { return win_; }

        //getter for width
        int getWidth() const { return currentW_; }
        int getHeight() const { return currentH_; }
        Vector2D getWindowSize() const { return Vector2D(static_cast<float>(currentW_), static_cast<float>(currentH_)); }

        /**
         * \brief Sets the window close flag.
         * \param shouldClose If true, the window will close on the next update.
         */
        void setWindowShouldClose(bool shouldClose);

    private:
        CoreEngine* core_ = nullptr;   //!< Owning engine (used for quit handling).
        const char* title_;            //!< Window title string.
        GLFWwindow* win_ = nullptr;    //!< Raw GLFW window handle.

        // Set defaults as fallback
        int currentW_ = 1920, currentH_ = 1080;      //!< Current Window width and height in pixels.
        int windowedW_ = 1920, windowedH_ = 1080;

        bool isFocused_ = true; // Windows Focus Loss Event
#ifndef BUILD_EDITOR
        int fullW_ = 1920, fullH_ = 1080;
        int x_{}, y_{};                    //!< Window top left x and y positions (0,0 will be top left of monitor)
#endif
        bool fullScreen_ = true;       //!< boolean to toggle between fullscreen and windowed mode. 
    };
} // namespace Framework
