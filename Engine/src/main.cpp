#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <iostream>
#include "Player.hpp"
#include <EntityStorage.hpp>

int main(void)
{
	Entity player = createPlayer(5.0f, 20.0f);
	std::cout << "Player entity created with ID: " 
        << player << " at " << position[player].x
        << ", " << position[player].y << std::endl ;

    GLFWwindow* window;

    // Initialize the GLFW library
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // Set OpenGL version to 4.5 Core Profile, just like in your tutorial
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);


    // Create a windowed mode window and its OpenGL context
    window = glfwCreateWindow(800, 600, "Owala", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Print the OpenGL version for confirmation
    std::cout << "Using OpenGL version: " << glGetString(GL_VERSION) << std::endl;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window))
    {
        // Render here
        glClearColor(0.2f, 0.3f, 0.8f, 1.0f); // A nice blue color
        glClear(GL_COLOR_BUFFER_BIT);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}