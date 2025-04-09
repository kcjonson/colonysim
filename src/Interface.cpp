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
    , currentFPS(0.0f) {
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
   // No update needed
}

void Interface::render(VectorGraphics& graphics, GLFWwindow* window) {
    // std::cout << "Rendering Interface..." << std::endl;
    
    // Get window dimensions
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    // std::cout << "Window size: " << width << "x" << height << std::endl;
    
    // Set up screen-space projection matrix
    // screen-space projection matrix transforms coordinates from screen space (pixels) to normalized device coordinates (NDC) for OpenGL rendering
    glm::mat4 projection(1.0f);
    projection[0][0] = 2.0f / width;   // Scale x
    projection[1][1] = -2.0f / height;  // Scale y (negative to flip y-axis)
    projection[3][0] = -1.0f;          // Translate x
    projection[3][1] = 1.0f;           // Translate y
    
    // Set projection for font renderer
    fontRenderer.setProjection(projection);
    
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
    
    // Use our Rectangle class instead of directly calling drawRectangle
    auto infoBox = std::make_shared<Rendering::Shapes::Rectangle>(
        boxPos,                                  // position
        glm::vec2(boxWidth, boxHeight),          // size
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),       // color
        0.0f                                     // z-index
    );
    
    // Draw the info box directly using VectorGraphics since we're still in the middle of migration
    infoBox->draw(graphics);
    
    // Create text strings
    std::stringstream cursorText;
    cursorText << "Cursor: (" << std::fixed << std::setprecision(1) 
               << cursorWorldPosition.x << ", " << cursorWorldPosition.y << ")";
    
    std::stringstream fpsText;
    fpsText << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
    
    // Draw text - for now we'll continue using the fontRenderer until we fully migrate to the new system
    fontRenderer.renderText(cursorText.str(), glm::vec2(left + padding, top + padding), fontScale, glm::vec3(1.0f, 1.0f, 1.0f));
    fontRenderer.renderText(fpsText.str(), glm::vec2(left + padding, top + padding + lineHeight), fontScale, glm::vec3(1.0f, 1.0f, 1.0f));
    
    // std::cout << "Interface rendering complete" << std::endl;
} 