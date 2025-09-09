// include/Framework/Graphics.h
#pragma once
#include "ISystem.h"
#include <vector>

namespace Framework {

    struct Vec2 { float x, y; };
    struct Color { float r, g, b, a; };

    class WindowsSystem;

    class Graphics : public ISystem {
    public:
        explicit Graphics(WindowsSystem& win) : win_(&win) {}

        void Initialize() override;
        void Update(float dt) override;
        void Shutdown() override {}

        // --- Frontend-friendly API ---
        // Call these from your game code (no OpenGL knowledge needed).
        void beginFrame(const Color& clear = { 0.1f,0.1f,0.12f,1.0f });
        void drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c,
            const Color& color);
        // endFrame is optional; Graphics will flush on Update()
        void endFrame();

    private:
        WindowsSystem* win_ = nullptr;

        // GPU objects
        unsigned vao_ = 0, vbo_ = 0, prog_ = 0;

        // CPU-side triangle vertex buffer for this frame
        struct Vtx { float x, y; float r, g, b, a; };
        std::vector<Vtx> frameVerts_;

        // helpers
        unsigned makeShader(unsigned type, const char* src);
        unsigned makeProgram(const char* vs, const char* fs);

        // clear color to use on this frame
        Color clearColor_{ 0.1f,0.1f,0.12f,1.0f };
    };

} // namespace Framework
