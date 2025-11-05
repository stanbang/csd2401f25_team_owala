/*********************************************************************//**
 * \file   WindowsSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (100%)
 *
 * \brief
 *      Implements the WindowsSystem class responsible for managing
 *      the application window and GLFW context lifecycle.
 *
 * \details
 *      The WindowsSystem handles:
 *        - GLFW initialization and OpenGL context creation.
 *        - Window creation, resizing, and event polling.
 *        - Graceful shutdown and resource cleanup.
 *
 *      The system is platform-agnostic but includes special handling
 *      for macOS compatibility (`GLFW_OPENGL_FORWARD_COMPAT`).
 *
 *      Workflow:
 *        1. Initialize GLFW and create an OpenGL window context.
 *        2. Poll window events each frame (close, resize, etc.).
 *        3. Handle quit requests through the Core system.
 *        4. Destroy window and terminate GLFW on shutdown.
 *
 * \copyright
 *      Copyright (C) 2025 DigiPen Institute of Technology.
 *      Reproduction or disclosure of this file or its contents without the prior
 *      written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#include "WindowsSystem.h"
#include "Core.h"
#include <cstdio>
#include <GLFW/glfw3.h>

namespace Framework {

	/**
	 * \brief Initializes the GLFW library and creates a window.
	 *
	 * Sets up OpenGL context version 3.3 (Core Profile), maximizes
	 * the created window, and enables vertical sync. If initialization
	 * fails, the Core system is instructed to terminate the program.
	 */
	void WindowsSystem::Initialize() {
		if (!glfwInit()) {
			std::fprintf(stderr, "GLFW init failed\n");
			core_->RequestQuit();
			return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		win_ = glfwCreateWindow(w_, h_, title_, nullptr, nullptr);
		glfwMaximizeWindow(win_);

		if (!win_) {
			std::fprintf(stderr, "CreateWindow failed\n");
			core_->RequestQuit();
			return;
		}

		glfwMakeContextCurrent(win_);
		glfwSwapInterval(1); // Enable V-Sync
	}

	/**
	 * \brief Polls for window events and checks for user exit.
	 *
	 * Called once per frame to process input and system messages.
	 * If the window close event is detected, it triggers a quit
	 * request in the Core system.
	 */
	void WindowsSystem::Update() {
		glfwPollEvents();
		if (win_ && glfwWindowShouldClose(win_))
			core_->RequestQuit();
	}

	/**
	 * \brief Destroys the GLFW window and terminates the library.
	 *
	 * Ensures safe shutdown and releases all GLFW-related resources.
	 */
	void WindowsSystem::Shutdown() {
		if (win_) {
			glfwDestroyWindow(win_);
			win_ = nullptr;
		}
		glfwTerminate();
	}

} // namespace Framework
