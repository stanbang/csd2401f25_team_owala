#pragma once
#include "ISystem.h"
#include <GL/glew.h>

namespace Framework {
	class WindowsSystem;

	struct Vec2 { float x, y; };
	struct Color { float r, g, b, a; };

	class Graphics : public ISystem {
	public:
		explicit Graphics(WindowsSystem& win) : win_(&win) {}

		void Initialize() override;
		void Update(float dt) override;
		void Shutdown() override {}

		// Frame helpers
		void beginFrame(const Color& clear);
		void drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Color& color);

		WindowsSystem* windowSystem() const { return win_; }

	private:
		WindowsSystem* win_{};
		float t_ = 0.f;

		// GL state for immediate triangles
		GLuint prog_ = 0;   // shader program
		GLuint vao_ = 0;   // vertex array
		GLuint vbo_ = 0;   // vertex buffer
	};
} // namespace Framework
