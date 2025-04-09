#include "Interface.h"
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include <GLFW/glfw3.h>

Interface::Interface()
    : cursorWorldPosition(0.0f, 0.0f)
    , currentFPS(0.0f)
    , fpsUpdateTimer(0.0f)
    , frameCount(0) {
}

void Interface::update(float deltaTime) {
    // Update FPS counter
    fpsUpdateTimer += deltaTime;
    frameCount++;

    if (fpsUpdateTimer >= 0.5f) { // Update FPS display every 0.5 seconds
        currentFPS = frameCount / fpsUpdateTimer;
        frameCount = 0;
        fpsUpdateTimer = 0.0f;
    }
}

void Interface::render(VectorGraphics& graphics, GLFWwindow* window) {
    // Create info box background
    const float padding = 10.0f;
    const float lineHeight = 20.0f;
    const float boxWidth = 200.0f;
    const float boxHeight = 60.0f;
    
    // Get window dimensions
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Calculate box position (top right corner)
    glm::vec2 boxPos(width - boxWidth - padding, padding);
    
    // Draw semi-transparent black background
    graphics.drawRectangle(boxPos, glm::vec2(boxWidth, boxHeight), glm::vec4(0.0f, 0.0f, 0.0f, 0.5f));
    
    // Create text strings
    std::stringstream cursorText;
    cursorText << "Cursor: (" << std::fixed << std::setprecision(1) 
               << cursorWorldPosition.x << ", " << cursorWorldPosition.y << ")";
    
    std::stringstream fpsText;
    fpsText << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
    
    // Draw text
    graphics.drawText(cursorText.str(), boxPos + glm::vec2(padding, padding), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    graphics.drawText(fpsText.str(), boxPos + glm::vec2(padding, padding + lineHeight), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
} 