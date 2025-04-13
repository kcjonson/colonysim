#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include "Renderer.h"
#include "World.h"
#include "Camera.h"
#include "InputManager.h"
#include "Interface.h"
#include "Rendering/Layer.h"
#include "Entities.h"
#include <memory>
#include "GameState.h"
#include "Examples.h"

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

    // For render stats logging
    float timeSinceLastRenderLog = 0.0f;

    // Members in initialization order
    GLFWwindow* window;
    Camera camera;
    GameState gameState;
    World world;
    Entities entities;
    InputManager inputManager;
    Interface interface;
    Examples examples;
    bool isRunning;
}; 