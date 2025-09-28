/*****************************************************************//**
 * \file   WindowsSystem.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
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
        WindowsSystem(CoreEngine& core, const char* title, int w, int h)
            : core_(&core), title_(title), w_(w), h_(h) {
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
        void Update(float dt) override;

        /**
         * \brief Shut down the window system.
         * \details Destroys the GLFW window and releases associated resources.
         */
        void Shutdown() override;

        /**
         * \brief Accessor for the raw GLFW window pointer.
         * \return Pointer to the underlying GLFWwindow.
         */
        GLFWwindow* window() const { return win_; }

    private:
        CoreEngine* core_ = nullptr;   //!< Owning engine (used for quit handling).
        const char* title_;            //!< Window title string.
        int w_, h_;                    //!< Window width and height in pixels.
        GLFWwindow* win_ = nullptr;    //!< Raw GLFW window handle.
    };
} // namespace Framework
