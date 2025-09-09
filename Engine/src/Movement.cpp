#include "WindowsSystem.h"
#include "Movement.h"
#include <GLFW/glfw3.h>
#include <iostream>

extern GLFWwindow* g_window;

namespace Framework {

    static float moveX = 0.0f;
    static float moveY = 0.0f;
    static const float moveSpeed = 1.5f;

    bool IsKeyWPressed() {
        return g_window && glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS;
    }

    bool IsKeyAPressed() {
        return g_window && glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS;
    }

    bool IsKeySPressed() {
        return g_window && glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS;
    }

    bool IsKeyDPressed() {
        return g_window && glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS;
    }

    void UpdateMovement(float dt) {
        float currentMoveX = 0.0f;
        float currentMoveY = 0.0f;
        std::cout << "UpdateMovement called\n";
        if (IsKeyWPressed()) {
            currentMoveY += 1.0f;
            std::cout << "W is pressed. Moving Up.\n";
        }
        if (IsKeySPressed()) {
            currentMoveY -= 1.0f;
            std::cout << "S is pressed. Moving Down.\n";
        }
        if (IsKeyAPressed()) {
            currentMoveX -= 1.0f;
            std::cout << "A is pressed. Moving Left.\n";
        }
        if (IsKeyDPressed()) {
            currentMoveX += 1.0f;
            std::cout << "D is pressed. Moving Right.\n";
        }

        moveX += currentMoveX * moveSpeed * dt;
        moveY += currentMoveY * moveSpeed * dt;
    }

    float UpdatedMoveX() {
        return moveX; 
    }
    float UpdatedMoveY() {
        return moveY; 
    }

    void ResetMovement() {
        moveX = 0.0f;
        moveY = 0.0f;
    }
}

