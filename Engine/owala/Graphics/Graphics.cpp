#include "Graphics.h"
#include "WindowsSystem.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <iostream>
#include <variant>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Logger.h"

namespace Framework {

    // =========================
    // Shaders
    // =========================

    // Colored triangle (pos only)
    static const char* VS_COLOR = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    void main(){ gl_Position = vec4(aPos, 0.0, 1.0); }
    )";

    static const char* FS_COLOR = R"(#version 330 core
    uniform vec4 uColor;
    out vec4 FragColor;
    void main(){ FragColor = uColor; }
    )";

    // Textured quad (pos + uv)
    static const char* VS_TEX = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aUV;
    out vec2 vUV;
    void main(){ gl_Position = vec4(aPos, 0.0, 1.0); vUV = aUV; }
    )";

    static const char* FS_TEX = R"(#version 330 core
    in vec2 vUV;
    uniform sampler2D uTex;
    out vec4 FragColor;
    void main(){ FragColor = texture(uTex, vUV); }
    )";

    // Textured quad (pos + uv)
    static const char* VS_TEXT = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec2 aUV;
    out vec2 vUV;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        vUV = aUV;
    })";

    static const char* FS_TEXT = R"(#version 330 core
    in vec2 vUV;
    uniform sampler2D uTex;
    uniform vec4 uColor;
    out vec4 FragColor;
    void main() {
        float alpha = texture(uTex, vUV).r;
        FragColor = vec4(uColor.rgb, uColor.a * alpha);
    }
    )";

    // =========================
    // Helpers
    // =========================

    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        }
        return s;
    }

    static GLuint linkProgram(const char* vsSrc, const char* fsSrc) {
        GLuint v = compile(GL_VERTEX_SHADER, vsSrc);
        GLuint f = compile(GL_FRAGMENT_SHADER, fsSrc);
        GLuint p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        glLinkProgram(p);
        glDeleteShader(v);
        glDeleteShader(f);
        GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
            std::fprintf(stderr, "Program link error:\n%s\n", log);
        }
        return p;
    }

    // =========================
    // ISystem
    // =========================

    void Graphics::Initialize() {
        // GLEW must be initialized after a valid GL context is current (WindowsSystem did that)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::fprintf(stderr, "GLEW init failed\n");
        }

        // Set initial viewport
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(win_->window(), &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);

        screenWidth = fbw;
        screenHeight = fbh;

        // ----- Colored triangle pipeline -----
        progCol_ = linkProgram(VS_COLOR, FS_COLOR);

        glGenVertexArrays(1, &vaoCol_);
        glBindVertexArray(vaoCol_);

        glGenBuffers(1, &vboCol_);
        glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
        // allocate room for 3 verts (x,y)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 2 * 3, nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

        // ----- Textured quad pipeline -----
        progTex_ = linkProgram(VS_TEX, FS_TEX);

        glGenVertexArrays(1, &vaoTex_);
        glBindVertexArray(vaoTex_);

        glGenBuffers(1, &vboTex_);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex_);
        // 6 verts * (x,y,u,v)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
        glEnableVertexAttribArray(1); // aUV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

        // ----- Text rendering pipeline -----
        progText_ = linkProgram(VS_TEXT, FS_TEXT);

        glGenVertexArrays(1, &vaoText_);
        glBindVertexArray(vaoText_);

        glGenBuffers(1, &vboText_);
        glBindBuffer(GL_ARRAY_BUFFER, vboText_);
        // 6 verts * (x,y,u,v) per char
        // Allocate room for a reasonable number of characters, e.g., 256
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4 * 256, nullptr, GL_STREAM_DRAW);

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
        glEnableVertexAttribArray(1); // aUV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

        // Unbind
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void Graphics::Update(float dt) {
        t_ += dt;
        // Present the frame (anything drawn since beginFrame)
        glfwSwapBuffers(win_->window());
    }

    void Graphics::Shutdown() {
        if (vboCol_) { glDeleteBuffers(1, &vboCol_); vboCol_ = 0; }
        if (vaoCol_) { glDeleteVertexArrays(1, &vaoCol_); vaoCol_ = 0; }
        if (progCol_) { glDeleteProgram(progCol_); progCol_ = 0; }

        if (vboTex_) { glDeleteBuffers(1, &vboTex_); vboTex_ = 0; }
        if (vaoTex_) { glDeleteVertexArrays(1, &vaoTex_); vaoTex_ = 0; }
        if (progTex_) { glDeleteProgram(progTex_); progTex_ = 0; }

        if (vboText_) { glDeleteBuffers(1, &vboText_); vboText_ = 0; }
        if (vaoText_) { glDeleteVertexArrays(1, &vaoText_); vaoText_ = 0; }
        if (progText_) { glDeleteProgram(progText_); progText_ = 0; }
    }

    // =========================
    // Colored triangle path
    // =========================

    void Graphics::beginFrame(const Color& clear) {
        glClearColor(clear.r, clear.g, clear.b, clear.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Graphics::drawTriangle(const Vector2D& a, const Vector2D& b, const Vector2D& c,
        const Color& color) {
        const float verts[6] = { a.x, a.y, b.x, b.y, c.x, c.y };

        glUseProgram(progCol_);
        GLint loc = glGetUniformLocation(progCol_, "uColor");
        glUniform4f(loc, color.r, color.g, color.b, color.a);

        glBindVertexArray(vaoCol_);
        glBindBuffer(GL_ARRAY_BUFFER, vboCol_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        // optional cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void Graphics::drawRectangle(const Vector2D& min, const Vector2D& max, const Color& color) {
        Vector2D p1 = { min.x, min.y }; // Bottom-left
        Vector2D p2 = { max.x, min.y }; // Bottom-right
        Vector2D p3 = { min.x, max.y }; // Top-left
        Vector2D p4 = { max.x, max.y }; // Top-right

        drawTriangle(p1, p2, p3, color);
        drawTriangle(p3, p2, p4, color);
    }

    // =========================
    // Texture path (.tex from your tutorial)
    // =========================

    Texture Graphics::loadTextureFromTex(const char* path) {
        // expects 256x256 RGBA raw (as in your tutorial)
        const int width = 256, height = 256, bpp = 4;
        const size_t total = size_t(width) * height * bpp;

        std::ifstream f(path, std::ios::binary);
        if (!f) {
            std::fprintf(stderr, "Failed to open %s\n", path);
            return 0;
        }

        std::vector<unsigned char> buf(total);
        f.read(reinterpret_cast<char*>(buf.data()), total);
        if (!f) {
            std::fprintf(stderr, "Short read on %s\n", path);
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        // allocate & upload
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, buf.data());
        // default params
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    void Graphics::setWrap(Texture tex, int wrapS, int wrapT) {
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Graphics::setAlphaBlend(bool on) {
        if (on) { glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
        else { glDisable(GL_BLEND); }
    }

    void Graphics::drawSprite(Texture& tex, const Vector2D& center,
        const Vector2D& halfSize, float rotation, const Vector2D& minUV, const Vector2D& maxUV) {


        // two triangles (6 verts), each vert = (x,y,u,v)
        const float cx = center.x, cy = center.y;
        const float hx = halfSize.x, hy = halfSize.y;

        const float c = std::cos(rotation);
        const float s = std::sin(rotation);

        auto rot = [&](float x, float y) -> Vector2D {
            return { cx + c * x - s * y, cy + s * x + c * y };
            };

        // CCW quad corners
        Vector2D p0 = rot(-hx, -hy); // uv 0,0
        Vector2D p1 = rot(hx, -hy); // uv 1,0
        Vector2D p2 = rot(hx, hy); // uv 1,1
        Vector2D p3 = rot(-hx, hy); // uv 0,1

        const float verts[] = {
            // x,   y,    u,   v
            p0.x, p0.y,  minUV.x, minUV.y,
            p1.x, p1.y,  maxUV.x, minUV.y,
            p2.x, p2.y,  maxUV.x, maxUV.y,

            p0.x, p0.y,  minUV.x, minUV.y,
            p2.x, p2.y,  maxUV.x, maxUV.y,
            p3.x, p3.y,  minUV.x, maxUV.y,
        };

        glEnable(GL_BLEND);
        glUseProgram(progTex_);
        glBindVertexArray(vaoTex_);
        glBindBuffer(GL_ARRAY_BUFFER, vboTex_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        const GLint loc = glGetUniformLocation(progTex_, "uTex");
        glUniform1i(loc, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        // optional cleanup
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    Texture Graphics::loadTextureFromPNG(const char* path, bool flipY, bool srgb)
    {
        if (flipY) stbi_set_flip_vertically_on_load(1); else stbi_set_flip_vertically_on_load(0);

        int w = 0, h = 0, n = 0;
        unsigned char* data = stbi_load(path, &w, &h, &n, STBI_rgb_alpha); // force RGBA8
        if (!data) {
            std::fprintf(stderr, "stbi_load failed for '%s': %s\n", path, stbi_failure_reason());
            return 0;
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const GLint internal = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        // reasonable defaults; you can change later
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);   // or GL_LINEAR_MIPMAP_LINEAR + glGenerateMipmap
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);
        return tex;
    }

    Texture Graphics::loadTextureFromFile(const char* path, bool /*unused*/) {
        // naive: choose by extension
        std::string p = path ? path : "";
        auto dot = p.find_last_of('.');
        if (dot != std::string::npos) {
            std::string ext = p.substr(dot + 1);
            for (char& c : ext) c = (char)std::tolower((unsigned char)c);

            // pass all required args to the PNG loader
            if (ext == "png") return loadTextureFromPNG(path, /*flipY*/ true, /*srgb*/ false);
            if (ext == "tex") return loadTextureFromTex(path);
        }

        // default to PNG if no/unknown extension
        return loadTextureFromPNG(path, /*flipY*/ true, /*srgb*/ false);
    }

    void Graphics::drawLine(const Vector2D& start, const Vector2D& end, const Color& color) {
        // Use the colored triangle pipeline
        glUseProgram(progCol_);
        GLint loc = glGetUniformLocation(progCol_, "uColor");
        glUniform4f(loc, color.r, color.g, color.b, color.a);

        // Create a thin rectangle (line) between start and end points
        Vector2D dir = { end.x - start.x, end.y - start.y };
        float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);

        if (length < 0.001f) return; // Too short

        // Normalize and get perpendicular for line thickness
        dir.x /= length;
        dir.y /= length;
        Vector2D perp = { -dir.y, dir.x };

        const float thickness = 0.005f; // Line thickness
        Vector2D halfThickness = { perp.x * thickness, perp.y * thickness };

        // Create a thin quad for the line
        Vector2D p1 = { start.x - halfThickness.x, start.y - halfThickness.y };
        Vector2D p2 = { start.x + halfThickness.x, start.y + halfThickness.y };
        Vector2D p3 = { end.x - halfThickness.x, end.y - halfThickness.y };
        Vector2D p4 = { end.x + halfThickness.x, end.y + halfThickness.y };

        // Draw as two triangles
        drawTriangle(p1, p3, p2, color);
        drawTriangle(p2, p3, p4, color);
    }

    void Graphics::drawText(Font& font, const std::string& textToDraw, const Vector2D& pos, float scale, const Color& color) {
        // --- 1. SETUP OPENGL STATE ---
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(progText_);

        // Set the uniform for text color
        GLint colorLoc = glGetUniformLocation(progText_, "uColor");
        glUniform4f(colorLoc, color.r, color.g, color.b, color.a);

        // Bind the font's texture atlas
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, font.GetAtlasID());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
        glUniform1i(glGetUniformLocation(progText_, "uTex"), 0);

        glBindVertexArray(vaoText_);
        glBindBuffer(GL_ARRAY_BUFFER, vboText_);

        // --- 2. PREPARE FOR RENDERING ---
        std::vector<float> vertices;
        vertices.reserve(textToDraw.length() * 6 * 4); // Reserve memory for vertices

        // Convert the starting position from NDC (-1 to 1) to screen pixels (0 to screenWidth/Height)
        // We'll use this as the starting cursor position (x). The y position is the baseline.
        float x = (pos.x + 1.0f) * 0.5f * screenWidth;
        float y = (pos.y + 1.0f) * 0.5f * screenHeight;

        // --- 3. BUILD VERTICES FOR EACH CHARACTER ---
        for (char c_signed : textToDraw) {
            unsigned char c = static_cast<unsigned char>(c_signed);
            const auto& ch = font.GetCharacters().at(c);

            // Calculate the vertex positions for the character quad in PIXEL coordinates
            float xpos = x + ch.Bearing.x * scale;
            float ypos = y - (ch.Size.y - ch.Bearing.y) * scale; // Baseline is at the top of the letters
            float w = ch.Size.x * scale;
            float h = ch.Size.y * scale;

            // Convert the final pixel coordinates back to NDC for rendering
            float ndc_xpos = (xpos / screenWidth) * 2.0f - 1.0f;
            float ndc_ypos = (ypos / screenHeight) * 2.0f - 1.0f;
            float ndc_w = (w / screenWidth) * 2.0f;
            float ndc_h = (h / screenHeight) * 2.0f;

            // Get UV coordinates
            float u = ch.TexCoords.x;
            float v = ch.TexCoords.y;
            float uvw = ch.TexCoords.z;
            float uvh = ch.TexCoords.w;

            // Add the 6 vertices for the character's quad to our buffer
            vertices.insert(vertices.end(), {
                ndc_xpos,         ndc_ypos + ndc_h,   u,         v,          // Top-left
                ndc_xpos,         ndc_ypos,           u,         v + uvh,    // Bottom-left
                ndc_xpos + ndc_w, ndc_ypos,           u + uvw,   v + uvh,    // Bottom-right

                ndc_xpos,         ndc_ypos + ndc_h,   u,         v,          // Top-left
                ndc_xpos + ndc_w, ndc_ypos,           u + uvw,   v + uvh,    // Bottom-right
                ndc_xpos + ndc_w, ndc_ypos + ndc_h,   u + uvw,   v           // Top-right
                });

            // Advance cursor for the next glyph. Advance is measured in 1/64 pixels.
            x += (ch.Advance >> 6) * scale;
        }

        // --- 4. DRAW THE VERTICES ---
        // Upload the vertex data to the VBO
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        // Draw all the characters in a single draw call
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 4));

        // --- 5. CLEANUP ---
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_BLEND);
    }

    std::vector<std::string> Graphics::WrapText(Font& font, const std::string& text, float scale, float maxWidth) {
        std::vector<std::string> lines;
        std::stringstream ss(text);
        std::string word;

        std::string currentLine;
        if (ss >> word) { // Get the first word
            currentLine = word;
        }

        while (ss >> word) {
            // Check if adding the next word exceeds the max width
            std::string testLine = currentLine + " " + word;
            if (font.GetStringWidth(testLine, scale) > maxWidth) {
                // The line is too long, so finalize the current line
                lines.push_back(currentLine);
                // Start a new line with the current word
                currentLine = word;
            }
            else {
                // The word fits, so add it to the current line
                currentLine = testLine;
            }
        }

        // Add the last line to the vector
        lines.push_back(currentLine);

        return lines;
    }

} // namespace Framework
