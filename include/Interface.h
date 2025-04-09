#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <string>
#include "VectorGraphics.h"
#include "FontRenderer.h"
#include "Rendering/Layer.h"
#include <GLFW/glfw3.h>

class VectorGraphics;
struct GLFWwindow;

class Interface {
public:
    Interface();
    ~Interface() = default;

    // Initialize font renderer
    bool initialize();
    
    // Initialize with graphics and window reference
    bool initializeGraphics(GLFWwindow* window);
    
    void update(float deltaTime);
    
    // Main render method for UI elements
    void render(VectorGraphics& graphics, const glm::mat4& projectionMatrix);

    // Setters for display information
    void setCursorWorldPosition(const glm::vec2& position) { cursorWorldPosition = position; }
    void setFPS(float fps) { currentFPS = fps; }

private:
    void renderInfoPanel(VectorGraphics& graphics);
    
    glm::vec2 cursorWorldPosition;
    float currentFPS;
    FontRenderer fontRenderer;
    
    // For organizing UI rendering
    std::shared_ptr<Rendering::Layer> uiLayer;
    GLFWwindow* targetWindow;
}; 