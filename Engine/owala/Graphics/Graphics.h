/*****************************************************************//**
 * \file   Graphics.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 * \secondary_author
 *
 * \brief
 *   Graphics subsystem wrapping OpenGL calls for basic 2D rendering.
 *   Provides colored primitives, textured sprites, and text drawing
 *   with font/layout helpers. Integrated with WindowsSystem for window
 *   context management.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without the prior
 *   written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "Vector2D.h"
#include <string>
#include <memory>
#include <variant>
#include "Font.h"

namespace Framework {

    /// \brief RGBA color struct (float channels, 0..1 range).
    struct Color { float r, g, b, a; };

    class WindowsSystem;                //!< Forward declaration to avoid heavy includes
    using Texture = unsigned;           //!< OpenGL texture handle (GLuint).
    using Texture_WeakPtr = std::weak_ptr<Texture>;   //!< Weak pointer to texture handle.
    using Texture_SharedPtr = std::shared_ptr<Texture>; //!< Shared pointer to texture handle.

    /**
     * \class Graphics
     * \brief OpenGL-based rendering system for 2D graphics.
     *
     * Responsibilities:
     * - Initialize OpenGL context and simple pipelines
     * - Draw colored primitives (triangles, rectangles)
     * - Load and render textured quads (sprites)
     * - Draw text with fonts and perform text wrapping
     *
     * Typical usage:
     * \code
     * Graphics gfx(winSys);
     * gfx.Initialize();
     *
     * gfx.beginFrame({0,0,0,1});
     * gfx.drawRectangle({0,0}, {100,100}, {1,0,0,1});
     * gfx.endFrame();
     * \endcode
     */
    class Graphics : public ISystem {
    public:
        /// \brief Construct graphics system bound to a WindowsSystem.
        explicit Graphics(WindowsSystem& win) : win_(&win) {}

        // ---------------- ISystem ----------------

        /// \brief Initialize GL context, shaders, VAOs/VBOs.
        void Initialize() override;

        /// \brief Per-frame update (handles buffer swap, tick).
        void Update(float dt) override;

        /// \brief Shutdown and release GL resources.
        void Shutdown() override;

        // ---------------- Screen info ----------------

        /// \return Current screen width in pixels.
        int getScreenWidth() const { return screenWidth; }

        /// \return Current screen height in pixels.
        int getScreenHeight() const { return screenHeight; }

        // ---------------- Primitive rendering ----------------

        /**
         * \brief Begin rendering a frame.
         * \param clear RGBA clear color for screen (defaults to dark grey).
         * \details Clears framebuffer to the given color. Call before any draw calls.
         */
        void beginFrame(const Color& clear = { 0.1f, 0.1f, 0.12f, 1.0f });

        /**
         * \brief Draw a filled colored triangle.
         * \param a Vertex position A in screen coordinates.
         * \param b Vertex position B in screen coordinates.
         * \param c Vertex position C in screen coordinates.
         * \param color Fill color (RGBA).
         */
        void drawTriangle(const Vector2D& a, const Vector2D& b, const Vector2D& c,
            const Color& color);

        /**
         * \brief End the frame. (Swap happens in Update()).
         */
        void endFrame() {}

        /**
         * \brief Draw a filled colored axis-aligned rectangle.
         * \param min Bottom-left corner.
         * \param max Top-right corner.
         * \param color Fill color.
         * \details Convenience for drawing AABB/collider debug visuals.
         */
        void drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color);

        // ---------------- Textures & Sprites ----------------

        /**
         * \brief Load a texture from a generic image file.
         * \param path Path to the file on disk.
         * \param srgb If true, treat as sRGB (gamma corrected).
         * \return OpenGL texture handle.
         */
        Texture loadTextureFromFile(const char* path, bool /*srgb*/ = false);

        /**
         * \brief Load a texture from a PNG file.
         * \param path Path to PNG file.
         * \param flipY Whether to flip vertically (common in GL vs image coords).
         * \param srgb  If true, treat as sRGB.
         * \return OpenGL texture handle.
         */
        Texture loadTextureFromPNG(const char* path, bool flipY, bool srgb);

        /**
         * \brief Load a texture from a custom .tex format (raw 256x256 RGBA).
         * \param path Path to .tex file.
         * \return OpenGL texture handle.
         */
        Texture loadTextureFromTex(const char* path);

        /**
         * \brief Set wrapping mode of a texture.
         * \param tex   Texture handle.
         * \param wrapS Wrap mode for S (U) coordinate (e.g., GL_CLAMP_TO_EDGE).
         * \param wrapT Wrap mode for T (V) coordinate.
         */
        void setWrap(Texture tex, int wrapS, int wrapT);

        /**
         * \brief Toggle alpha blending globally.
         * \param on True enables blending; false disables.
         * \details Uses standard GL_BLEND state.
         */
        void setAlphaBlend(bool on);

        /**
         * \brief Draw a textured sprite (quad).
         * \param tex      Texture handle to bind.
         * \param center   Sprite center position.
         * \param halfSize Half extents of sprite (width/2, height/2).
         * \param rotation Rotation in radians (optional).
         */
        void drawSprite(Texture& tex, const Vector2D& center,
            const Vector2D& halfSize, float rotation = 0.0f, const Vector2D& minUV = { 0,0 }, const Vector2D& maxUV = { 1,1 });



        void drawLine(const Vector2D& start, const Vector2D& end, const Color& color);

        // ---------------- Text ----------------

        /**
         * \brief Draw text on screen.
         * \param font   Font resource (glyph atlas).
         * \param text   UTF-8 string.
         * \param pos    Baseline position.
         * \param scale  Scale factor (1.0 = font’s base size).
         * \param color  RGBA text color.
         */
        void drawText(Font& font, const std::string& text,
            const Vector2D& pos, float scale, const Color& color);

        /**
         * \brief Word-wrap a long string to multiple lines based on width.
         * \param font     Font resource.
         * \param text     Input string.
         * \param scale    Font scale.
         * \param maxWidth Maximum width in pixels before wrapping.
         * \return Vector of lines (strings) after wrapping.
         */
        std::vector<std::string> WrapText(Font& font, const std::string& text,
            float scale, float maxWidth);

    private:
        WindowsSystem* win_ = nullptr; //!< Parent window system for GL context.
        int screenWidth = 1920;               //!< Current screen width.
        int screenHeight = 1080;              //!< Current screen height.

        // ---- Colored triangle pipeline ----
        unsigned progCol_ = 0; //!< Shader program (uColor).
        unsigned vaoCol_ = 0; //!< VAO.
        unsigned vboCol_ = 0; //!< VBO (positions only).

        // ---- Textured quad pipeline ----
        unsigned progTex_ = 0; //!< Shader program (uTex).
        unsigned vaoTex_ = 0; //!< VAO.
        unsigned vboTex_ = 0; //!< VBO (pos+uv, 6 verts).

        // ---- Text rendering pipeline ----
        unsigned progText_ = 0; //!< Shader program for text rendering.
        unsigned vaoText_ = 0; //!< VAO for text quads.
        unsigned vboText_ = 0; //!< VBO (pos+uv, 6 verts).

        float t_ = 0.f; //!< Optional tick counter (useful for animation).
    };

} // namespace Framework
