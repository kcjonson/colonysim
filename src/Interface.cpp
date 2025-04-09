#include "Interface.h"
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include <GLFW/glfw3.h>
#include <iostream>

Interface::Interface()
    : cursorWorldPosition(0.0f, 0.0f)
    , currentFPS(0.0f)
    , fpsUpdateTimer(0.0f)
    , frameCount(0) {
    std::cout << "Interface constructor called" << std::endl;
}

bool Interface::initialize() {
    std::cout << "Initializing Interface..." << std::endl;
    if (!fontRenderer.initialize()) {
        std::cerr << "Failed to initialize FontRenderer" << std::endl;
        return false;
    }
    std::cout << "Interface initialization complete" << std::endl;
    return true;
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
    std::cout << "Rendering Interface..." << std::endl;
    
    // Create info box background
    const float padding = 10.0f;
    const float lineHeight = 20.0f;
    const float boxWidth = 200.0f;
    const float boxHeight = 60.0f;
    
    // Get window dimensions
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    std::cout << "Window size: " << width << "x" << height << std::endl;
    
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
    
    // Set up screen-space projection matrix manually
    glm::mat4 projection(1.0f);
    projection[0][0] = 2.0f / width;  // Scale x
    projection[1][1] = -2.0f / height; // Scale y (negative to flip y-axis)
    projection[3][0] = -1.0f;          // Translate x
    projection[3][1] = 1.0f;           // Translate y
    fontRenderer.setProjection(projection);
    
    // Draw text using FontRenderer
    fontRenderer.renderText(cursorText.str(), boxPos.x + padding, boxPos.y + padding, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    fontRenderer.renderText(fpsText.str(), boxPos.x + padding, boxPos.y + padding + lineHeight, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
    
    std::cout << "Interface rendering complete" << std::endl;
} 