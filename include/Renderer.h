#pragma once

#include <memory>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "VectorGraphics.h"
#include "Camera.h"
#include "InputManager.h"
#include "EntityManager.h"

class Renderer {
public:
    Renderer(int width, int height, const std::string& title);
    ~Renderer();

    void initialize();
    void render();
    bool shouldClose() const;
    void setWindowSize(int width, int height);

private:
    GLFWwindow* window;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<InputManager> inputManager;
    VectorGraphics vectorGraphics;
}; 