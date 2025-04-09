#include "Interface.h"
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Text.h"
#include <GLFW/glfw3.h>
#include <iostream>

Interface::Interface()
    : cursorWorldPosition(0.0f, 0.0f)
    , currentFPS(0.0f)
    , uiLayer(nullptr)
    , targetWindow(nullptr) {
    std::cout << "Interface constructor called" << std::endl;
    uiLayer = std::make_shared<Rendering::Layer>(1000.0f);
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

bool Interface::initializeGraphics(GLFWwindow* window) {
    targetWindow = window;
    return true;
}

void Interface::update(float deltaTime) {
   // No update needed
}

void Interface::render(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    // Set projection for font renderer
    fontRenderer.setProjection(projectionMatrix);
    
    // Render interface elements
    renderInfoPanel(graphics);
}

void Interface::renderInfoPanel(VectorGraphics& graphics) {
    // Create info box background
    const float fontScale = 0.3f;  // Adjusted for approximately 14pt
    const float padding = 10.0f;
    const float lineHeight = 20.0f;
    const float left = 90.0f;
    const float top = 90.0f;
    const float boxWidth = 200.0f;
    const float boxHeight = 60.0f;
    
    // Calculate box position
    // Since drawRectangle uses center-based positioning, we need to adjust the position
    glm::vec2 boxPos(left + boxWidth/2, top + boxHeight/2);
    
    // Draw the info box background
    graphics.drawRectangle(
        boxPos,
        glm::vec2(boxWidth, boxHeight),
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
    );
    
    // Create text strings
    std::stringstream cursorText;
    cursorText << "Cursor: (" << std::fixed << std::setprecision(1) 
               << cursorWorldPosition.x << ", " << cursorWorldPosition.y << ")";
    
    std::stringstream fpsText;
    fpsText << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
    
    // Draw text using the font renderer
    fontRenderer.renderText(cursorText.str(), glm::vec2(left + padding, top + padding), fontScale, glm::vec3(1.0f, 1.0f, 1.0f));
    fontRenderer.renderText(fpsText.str(), glm::vec2(left + padding, top + padding + lineHeight), fontScale, glm::vec3(1.0f, 1.0f, 1.0f));
} 