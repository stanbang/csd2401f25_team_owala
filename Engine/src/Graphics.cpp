// src/Framework/Graphics.cpp
#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>

namespace Framework {

    static const char* kVS = R"(#version 330 core
    layout(location=0) in vec2 aPos;
    layout(location=1) in vec4 aColor;
    out vec4 vColor;
    void main(){
      gl_Position = vec4(aPos, 0.0, 1.0);
      vColor = aColor;
    }
    )";

    static const char* kFS = R"(#version 330 core
    in vec4 vColor;
    out vec4 FragColor;
    void main(){ FragColor = vColor; }
    )";

    unsigned Graphics::makeShader(unsigned type, const char* src) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok = GL_FALSE; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(sh, 1024, nullptr, log);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        }
        return sh;
    }

    unsigned Graphics::makeProgram(const char* vs, const char* fs) {
        GLuint v = makeShader(GL_VERTEX_SHADER, vs);
        GLuint f = makeShader(GL_FRAGMENT_SHADER, fs);
        GLuint p = glCreateProgram();
        glAttachShader(p, v); glAttachShader(p, f);
        glLinkProgram(p);
        glDeleteShader(v); glDeleteShader(f);
        GLint ok = GL_FALSE; glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetProgramInfoLog(p, 1024, nullptr, log);
            std::fprintf(stderr, "Program link error:\n%s\n", log);
        }
        return p;
    }

    void Graphics::Initialize() {
        // GLEW must be initialized AFTER a context is current (WindowsSystem did that)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) std::fprintf(stderr, "GLEW init failed\n");

        // Create program + VAO/VBO once
        prog_ = makeProgram(kVS, kFS);

        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // sized on flush

        glEnableVertexAttribArray(0); // aPos
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)0);
        glEnableVertexAttribArray(1); // aColor
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vtx), (void*)(sizeof(float) * 2));

        // viewport
        int fbw = 0, fbh = 0; glfwGetFramebufferSize(win_->window(), &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
    }

    void Graphics::beginFrame(const Color& clear) {
        clearColor_ = clear;
        frameVerts_.clear();
    }

    void Graphics::drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c,
        const Color& col) {
        frameVerts_.push_back({ a.x,a.y, col.r,col.g,col.b,col.a });
        frameVerts_.push_back({ b.x,b.y, col.r,col.g,col.b,col.a });
        frameVerts_.push_back({ c.x,c.y, col.r,col.g,col.b,col.a });
    }

    void Graphics::endFrame() {
        // nothing needed here; flush happens in Update()
    }

    void Graphics::Update(float /*dt*/) {
        // clear
        glClearColor(clearColor_.r, clearColor_.g, clearColor_.b, clearColor_.a);
        glClear(GL_COLOR_BUFFER_BIT);

        // flush queued triangles (if any)
        if (!frameVerts_.empty()) {
            glUseProgram(prog_);
            glBindVertexArray(vao_);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER,
                (GLsizeiptr)(frameVerts_.size() * sizeof(Vtx)),
                frameVerts_.data(),
                GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (GLint)frameVerts_.size());
        }

        // present
        glfwSwapBuffers(win_->window());

        // ready for next frame (if game forgot to call beginFrame)
        frameVerts_.clear();
    }

} // namespace Framework
