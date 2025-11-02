/*****************************************************************//**
 * \file   Graphics.h
 *
 * \primary_author
 *      Ng Jordain Ying Zhe
 *
 * \brief
 *   Graphics subsystem wrapping OpenGL calls for basic 2D rendering.
 *   Provides colored primitives, textured sprites, and text drawing
 *   with batching and font/layout helpers.
 *
 * \copyright
 *   Copyright (C) 2025 DigiPen Institute of Technology.
 *   Reproduction or disclosure of this file or its contents without
 *   the prior written consent of DigiPen Institute of Technology is prohibited.
 *********************************************************************/

#pragma once
#include "ISystem.h"
#include "Vector2D.h"
#include "Font.h"
#include <string>
#include <memory>
#include <vector>

namespace Framework {

    // ------------------------------------------------------------------
    // Basic types
    // ------------------------------------------------------------------
    struct Color { float r, g, b, a; };
    class WindowsSystem;
    using Texture = unsigned;  //!< GLuint alias

    // ------------------------------------------------------------------
    // Sprite batching data structure
    // ------------------------------------------------------------------
    struct BatchedSprite {
        Vector2D topLeft;   //!< Top-left position in screen-space
        Vector2D size;      //!< Width/height in screen-space
        float rotation;     //!< Rotation in radians
        Vector2D minUV;     //!< Usually (0,0)
        Vector2D maxUV;     //!< Usually (1,1)
    };

    // ------------------------------------------------------------------
    // Graphics system
    // ------------------------------------------------------------------
    class Graphics : public ISystem {
    public:
        explicit Graphics(WindowsSystem& win) : win_(&win) {}

        // -------- ISystem interface --------
        void Initialize() override;
        void Update() override;
        void Shutdown() override;

        // -------- Screen info --------
        int  getScreenWidth()  const { return screenWidth; }
        int  getScreenHeight() const { return screenHeight; }

        // -------- Primitive rendering --------
        void beginFrame(const Color& clear = { 0.1f, 0.1f, 0.12f, 1.0f });
        void drawTriangle(const Vector2D& a, const Vector2D& b, const Vector2D& c, const Color& color);
        void drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color);
        void drawLine(const Vector2D& start, const Vector2D& end, const Color& color);
        void endFrame() {}

        // -------- Texture loading --------
        Texture loadTextureFromFile(const char* path, bool srgb = false);
        Texture loadTextureFromPNG(const char* path, bool flipY, bool srgb);
        Texture loadTextureFromTex(const char* path);
        void    setWrap(Texture tex, int wrapS, int wrapT);
        void    setAlphaBlend(bool on);

        // -------- Sprite rendering (single) --------
        void drawSprite(Texture& tex, const Vector2D& center,
            const Vector2D& halfSize, float rotation = 0.0f,
            const Vector2D& minUV = { 0,0 }, const Vector2D& maxUV = { 1,1 });

        void drawSpriteEasy(Texture& tex, const Vector2D& topLeft,
            const Vector2D& size, float rotation = 0.0f,
            const Vector2D& minUV = { 0,0 }, const Vector2D& maxUV = { 1,1 });

        // -------- Sprite rendering (batched) --------
        /**
         * \brief Draw a batch of sprites sharing the same texture in a single draw call.
         * \param tex      Texture handle to bind once.
         * \param sprites  Vector of BatchedSprite instances.
         * \details Significantly reduces draw calls by grouping quads per texture.
         */
        void drawSpriteBatch(Texture tex, const std::vector<BatchedSprite>& sprites);

        /**
         * \brief Lower-level variant of drawSpriteBatch for prebuilt vertex data.
         * \param tex Texture handle.
         * \param interleavedXYUV Pointer to interleaved x,y,u,v data (6 verts per sprite).
         * \param spriteCount Number of sprites in the array.
         */
        void drawSpriteBatchRaw(Texture tex, const float* interleavedXYUV, size_t spriteCount);
        void drawSpriteBatchChunk(Texture tex, const std::vector<BatchedSprite>& sprites);
        // -------- Text --------
        void drawText(Font& font, const std::string& text,
            const Vector2D& pos, float scale, const Color& color);

        std::vector<std::string> WrapText(Font& font, const std::string& text,
            float scale, float maxWidth);

        // ----- Frame Buffer stuff (For Game Viewport) -----
        struct FramebufferSpecs {
            unsigned width = 800;
            unsigned height = 600;
		};

        class Framebuffer {
        private:
            FramebufferSpecs specs;
			unsigned rendererID = 0;
			unsigned colorAttachment = 0;
			unsigned depthAttachment = 0;
        public:
            /* Constructors */
            Framebuffer(const FramebufferSpecs& specs);
            Framebuffer(unsigned width, unsigned height);
            ~Framebuffer();
            
            /* Getters */
			const FramebufferSpecs& GetSpecifications() const { return specs; }
			unsigned GetRendererID() const { return rendererID; }
			unsigned GetColorAttachment() const { return colorAttachment; }

            /* APIs */
            void Bind() const;
            void Unbind() const;
            void Resize(FramebufferSpecs specifications);
			void Resize(unsigned width, unsigned height);
        };

        void CreateGameFramebuffer(unsigned width, unsigned height);
		void ResizeGameFramebuffer(unsigned width, unsigned height);
		void BindGameFramebuffer() const;
		void UnbindGameFramebuffer() const;
        unsigned GetGameTexture() const { return gameFramebuffer_ ? gameFramebuffer_->GetColorAttachment() : 0; };

    private:
        WindowsSystem* win_ = nullptr;
        int screenWidth = 1920;
        int screenHeight = 1080;

        // Colored triangle pipeline
        unsigned progCol_ = 0;
        unsigned vaoCol_ = 0;
        unsigned vboCol_ = 0;

        // Textured sprite pipeline (now supports batching)
        unsigned progTex_ = 0;
        unsigned vaoTex_ = 0;
        unsigned vboTex_ = 0;

        // Text rendering pipeline
        unsigned progText_ = 0;
        unsigned vaoText_ = 0;
        unsigned vboText_ = 0;

        float t_ = 0.f;

        // Game Frame
		std::unique_ptr<Framebuffer> gameFramebuffer_;
    };

} // namespace Framework
