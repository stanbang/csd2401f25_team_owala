// Graphics.cpp
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <cmath>

namespace Framework {
	void Graphics::Initialize() {
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) std::fprintf(stderr, "GLEW init failed\n");
		int fbw = 0, fbh = 0; glfwGetFramebufferSize(win_->window(), &fbw, &fbh);
		glViewport(0, 0, fbw, fbh);
	}
	void Graphics::Update(float dt) {
		t_ += dt;
		float r = 0.2f + 0.2f * std::sin(t_ * 2.0f);
		float g = 0.3f + 0.2f * std::sin(t_ * 1.7f);
		float b = 0.8f + 0.2f * std::sin(t_ * 1.3f);
		glClearColor(r, g, b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(win_->window());
	}
} // namespace Framework
