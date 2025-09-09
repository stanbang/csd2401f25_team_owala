// WindowsSystem.cpp
#include "WindowsSystem.h"
#include "Core.h"
#include <cstdio>
#include <GLFW/glfw3.h>
#include "Movement.h"

GLFWwindow* g_window = nullptr;
namespace Framework {
	void WindowsSystem::Initialize() {
		if (!glfwInit()) { std::fprintf(stderr, "GLFW init failed\n"); core_->RequestQuit(); return; }
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
		win_ = glfwCreateWindow(w_, h_, title_, nullptr, nullptr);
		if (!win_) { std::fprintf(stderr, "CreateWindow failed\n"); core_->RequestQuit(); return; }
		glfwMakeContextCurrent(win_);
		glfwSwapInterval(1);
		g_window = win_;
	}
	void WindowsSystem::Update(float) {
		glfwPollEvents();
		if (win_ && glfwWindowShouldClose(win_)) core_->RequestQuit();
	}
	void WindowsSystem::Shutdown() {
		g_window = nullptr;
		if (win_) { glfwDestroyWindow(win_); win_ = nullptr; }
		glfwTerminate();
	}
} // namespace Framework
