#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <string>
#include "VectorGraphics.h"
#include "FontRenderer.h"
#include <GLFW/glfw3.h>

class VectorGraphics;
struct GLFWwindow;

class Interface {
public:
    Interface();
    ~Interface() = default;

    bool initialize();
    void update(float deltaTime);
    void render(VectorGraphics& graphics, GLFWwindow* window);

    // Setters for display information
    void setCursorWorldPosition(const glm::vec2& position) { cursorWorldPosition = position; }
    void setFPS(float fps) { currentFPS = fps; }

private:
    glm::vec2 cursorWorldPosition;
    float currentFPS;
    FontRenderer fontRenderer;
}; 