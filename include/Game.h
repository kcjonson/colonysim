#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include "World.h"
#include "Camera.h"
#include "InputManager.h"

class Game {
public:
    Game();
    ~Game();

    void run();

private:
    void processInput();
    void update(float deltaTime);
    void render();

    // GLFW callbacks
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void mouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // Members in initialization order
    GLFWwindow* window;
    Camera camera;
    World world;
    InputManager inputManager;
    VectorGraphics vectorGraphics;
    bool isRunning;
}; 