#include "Graphics.h"
#include "WindowsSystem.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>

namespace Framework {

    // tiny helper
    static GLuint compile(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = GL_FALSE; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log);
            std::fprintf(stderr, "Shader compile error:\n%s\n", log);
        }
        return s;
    }

    void Graphics::Initialize() {
        // 1) Ensure a current context, then init GLEW
        glfwMakeContextCurrent(win_->window());
        glewExperimental = GL_TRUE;
        if (auto err = glewInit(); err != GLEW_OK) {
            std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(err));
            return;
        }
        // GLEW on core contexts can generate a spurious error once; clear it.
        while (glGetError() != GL_NO_ERROR) {}

        // (Optional) confirm we’re good
        std::fprintf(stderr, "OpenGL: %s\n", glGetString(GL_VERSION));

        // 2) Build a tiny pass-through pipeline
        const char* vs = R"(#version 330 core
        layout(location=0) in vec2 inPos;
        void main(){ gl_Position = vec4(inPos, 0.0, 1.0); }
    )";
        const char* fs = R"(#version 330 core
        out vec4 outColor;
        uniform vec4 uColor;
        void main(){ outColor = uColor; }
    )";
        GLuint v = compile(GL_VERTEX_SHADER, vs);
        GLuint f = compile(GL_FRAGMENT_SHADER, fs);

        prog_ = glCreateProgram();
        glAttachShader(prog_, v);
        glAttachShader(prog_, f);
        glLinkProgram(prog_);
        glDeleteShader(v);
        glDeleteShader(f);

        // 3) VAO/VBO (GL 3.3 path – portable)
        glGenVertexArrays(1, &vao_);
        glBindVertexArray(vao_);

        glGenBuffers(1, &vbo_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // size set per draw

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, (void*)0);

        // 4) Set viewport to framebuffer size
        int fbw = 0, fbh = 0;
        glfwGetFramebufferSize(win_->window(), &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
    }

    void Graphics::beginFrame(const Color& clear) {
        glClearColor(clear.r, clear.g, clear.b, clear.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Graphics::drawTriangle(const Vec2& a, const Vec2& b, const Vec2& c, const Color& color) {
        float verts[6] = { a.x,a.y, b.x,b.y, c.x,c.y };

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STREAM_DRAW);

        glUseProgram(prog_);
        GLint loc = glGetUniformLocation(prog_, "uColor");
        glUniform4f(loc, color.r, color.g, color.b, color.a);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void Graphics::Update(float dt) {
        (void)dt;
        // If your WindowsSystem polls events, just swap here.
        // Otherwise you can add: glfwPollEvents();
        glfwSwapBuffers(win_->window());
    }

} // namespace Framework
