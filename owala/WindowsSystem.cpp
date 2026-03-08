/*********************************************************************//**
 * \file   WindowsSystem.cpp
 *
 * \primary_author
 *      Ng Jordain Ying Zhe (40%)
 * \secondary_author
 *		Yar Xuan (50%)
 *		Ng Jing Wen (10%)
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
#include "pch.h"
#include "WindowsSystem.h"
#include "Core.h"
#include "Logger.h"

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
			Logger::Get().Error(DebugType::WindowsSystem) << "GLFW init failed\n";
			core_->RequestQuit();
			return;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
		
#ifndef BUILD_EDITOR
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable window resizing in game mode

		// --- Monitor and Video Mode Setup for Full-Screen ---
		// Get the primary monitor
		GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
		if (!primary_monitor) {
			Logger::Get().Error(DebugType::WindowsSystem) << "Failed to get primary monitor\n";
			core_->RequestQuit();
			return;
		}

		// Get the current video mode of the monitor
		const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
		if (!mode) {
			Logger::Get().Error(DebugType::WindowsSystem) << "Failed to get video mode\n";
			core_->RequestQuit();
			return;
		}

		// set full screen dimensions
		fullW_ = mode->width;
		fullH_ = mode->height;
		// set default window position to center
		x_ = (fullW_ - windowedW_) / 2;
		y_ = (fullH_ - windowedH_) / 2;

		if (fullScreen_) {
			// Pass the monitor pointer (primary_monitor) to glfwCreateWindow
			win_ = glfwCreateWindow(fullW_, fullH_, title_, primary_monitor, nullptr);
			currentW_ = fullW_; currentH_ = fullH_;
		}
		else {
			win_ = glfwCreateWindow(windowedW_, windowedH_, title_, nullptr, nullptr);
			currentW_ = windowedW_; currentH_ = windowedH_;
		}
#endif

#ifdef BUILD_EDITOR
		win_ = glfwCreateWindow(windowedW_, windowedH_, title_, nullptr, nullptr);
		glfwMaximizeWindow(win_); 
		currentW_ = windowedW_; currentH_ = windowedH_;
#endif


		if (!win_) {
			Logger::Get().Error(DebugType::WindowsSystem) << "CreateWindow failed\n";
			core_->RequestQuit();
			return;
		}

		glfwSetWindowUserPointer(win_, this); // this pointer so static callbacks can access it
		glfwSetWindowFocusCallback(win_, WindowFocusCallback);

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
		if (win_) {
			// Poll GLFW directly
			isFocused_ = (glfwGetWindowAttrib(win_, GLFW_FOCUSED) == GLFW_TRUE); // for callback conflict with IMGUI

			if (glfwWindowShouldClose(win_)) {
				core_->RequestQuit();
			}
		}
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

#ifndef BUILD_EDITOR
	/**
	 * \brief Toggle between full screen and windowed mode.
	 */
	void WindowsSystem::ToggleFullScreen() {
		Logger::Get().Info(DebugType::WindowsSystem) << "Toggled Full Screen Mode: " << !fullScreen_ << "\n";
		GLFWmonitor* current_monitor = glfwGetWindowMonitor(win_);

		if (current_monitor && fullScreen_) {
			// --- Currently Full-Screen: Switch to Windowed Mode ---
			glfwSetWindowMonitor(win_, nullptr, x_, y_, windowedW_, windowedH_, 0);
			fullScreen_ = false;
			currentW_ = windowedW_; currentH_ = windowedH_;
		}
		else {
			// --- Currently Windowed: Switch to Full-Screen Mode ---
			// Get current windowed size and position (needed to restore later)
			glfwGetWindowPos(win_, &x_, &y_);
			glfwGetWindowSize(win_, &windowedW_, &windowedH_);

			//  Get the primary monitor and its video mode
			GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);

			//  Set the window to full-screen using the monitor's mode
			glfwSetWindowMonitor(win_, primary_monitor,
				0, 0, // Position is always (0, 0) for full-screen
				mode->width, mode->height,
				mode->refreshRate);

			fullScreen_ = true;
			fullW_ = mode->width; fullH_= mode->height;
			currentW_ = fullW_; currentH_ = fullH_;
		}

		// Crucial: Update the OpenGL Viewport after any size change
		int fb_w, fb_h;
		glfwGetFramebufferSize(win_, &fb_w, &fb_h);
		glViewport(0, 0, fb_w, fb_h);
	}
#endif

	void WindowsSystem::setWindowShouldClose(bool shouldClose) {
		if (win_) {
			glfwSetWindowShouldClose(win_, shouldClose ? GLFW_TRUE : GLFW_FALSE);
		}
	}

	void WindowsSystem::WindowFocusCallback(GLFWwindow* window, int focused) {
		// Set Windows Callback Boolean 
		// The cleanest way in GLFW is to store the "this" pointer in the window user pointer.

		WindowsSystem* sys = static_cast<WindowsSystem*>(glfwGetWindowUserPointer(window));
		if (sys) {
			sys->isFocused_ = (focused == GLFW_TRUE);

			// OPTIONAL: Log for debug
			// Logger::Get().Info(DebugType::WindowsSystem) << "Window Focus: " << sys->isFocused_ << "\n";
		}
	}

} // namespace Framework
