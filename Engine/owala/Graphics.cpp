#include "Graphics.h"
#include "WindowsSystem.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <fstream>
#include <vector>
#include <cmath>

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

    void Graphics::drawSprite(Texture tex, const Vector2D& center,
        const Vector2D& halfSize, float rotation) {
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
            p0.x, p0.y,  0.f, 0.f,
            p1.x, p1.y,  1.f, 0.f,
            p2.x, p2.y,  1.f, 1.f,

            p0.x, p0.y,  0.f, 0.f,
            p2.x, p2.y,  1.f, 1.f,
            p3.x, p3.y,  0.f, 1.f,
        };

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

} // namespace Framework
