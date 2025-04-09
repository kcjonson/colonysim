#pragma once

#include <glm/glm.hpp>
#include <string>
#include "VectorGraphics.h"
#include <GLFW/glfw3.h>

class Interface {
public:
    Interface();
    ~Interface() = default;

    void update(float deltaTime);
    void render(VectorGraphics& graphics, GLFWwindow* window);

    // Setters for display information
    void setCursorWorldPosition(const glm::vec2& position) { cursorWorldPosition = position; }
    void setFPS(float fps) { currentFPS = fps; }

private:
    glm::vec2 cursorWorldPosition;
    float currentFPS;
    float fpsUpdateTimer;
    int frameCount;
}; 