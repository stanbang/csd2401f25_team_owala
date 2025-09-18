#pragma once
#include "ISystem.h"
#include "Vector2D.h"
#include <string>
#include <memory>
#include <variant>

namespace Framework {

    // Simple POD types your game/devs can use without knowing OpenGL
    //struct Vec2 { float x, y; };
    struct Color { float r, g, b, a; };

    class WindowsSystem;                // fwd-declared to avoid heavy includes
    using Texture = unsigned;           // OpenGL texture handle (GLuint)
	using Texture_WeakPtr = std::weak_ptr<Texture>; // Weak pointer to texture
	using Texture_SharedPtr = std::shared_ptr<Texture>; // Shared pointer to texture

    class Graphics : public ISystem {
    public:
        explicit Graphics(WindowsSystem& win) : win_(&win) {}

        // ISystem
        void Initialize() override;
        void Update(float dt) override;
        void Shutdown() override;

        // ---- Colored triangle (immediate style) ----
        void beginFrame(const Color& clear = { 0.1f, 0.1f, 0.12f, 1.0f });
        void drawTriangle(const Vector2D& a, const Vector2D& b, const Vector2D& c,
            const Color& color);
        void endFrame() {} // swap happens in Update()

        // Helper function to easily draw rectangles for our colliders
        void drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color);

        // Helper function to easily draw rectangles for our colliders
       // void drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color);

        // ---- Textured sprites (.tex format from your tutorial) ----
        Texture loadTextureFromFile(const char* path, bool /*srgb*/ = false);
        Texture loadTextureFromPNG(const char* path, bool flipY, bool srgb);
        Texture loadTextureFromTex(const char* path);               // expects 256x256 RGBA raw
        void    setWrap(Texture tex, int wrapS, int wrapT);         // GL_* wrap enums (e.g., GL_CLAMP_TO_EDGE)
        void    setAlphaBlend(bool on);                             // toggles GL_BLEND
        void    drawSprite(std::variant<std::shared_ptr<Texture>> tex, const Vector2D& center,
            const Vector2D& halfSize, float rotation = 0.0f);

    private:
        WindowsSystem* win_ = nullptr;

        // Colored-triangle pipeline
        unsigned progCol_ = 0;  // shader program (uColor)
        unsigned vaoCol_ = 0;  // VAO
        unsigned vboCol_ = 0;  // VBO (pos only)

        // Textured-quad pipeline
        unsigned progTex_ = 0;  // shader program (uTex)
        unsigned vaoTex_ = 0;  // VAO
        unsigned vboTex_ = 0;  // VBO (pos+uv, 6 verts)

        float t_ = 0.f;         // optional tick; used if you animate later
    };

} // namespace Framework
