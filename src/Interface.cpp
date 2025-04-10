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
    , targetWindow(nullptr)
    , infoPanelBackground(nullptr)
    , cursorPositionText(nullptr)
    , fpsText(nullptr) {
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
    
    // Initialize all UI components
    initializeUIComponents();
    
    return true;
}

void Interface::initializeUIComponents() {
    // Create info panel background
    glm::vec2 boxPos(INFO_PANEL_X + INFO_PANEL_WIDTH/2, INFO_PANEL_Y + INFO_PANEL_HEIGHT/2);
    infoPanelBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        boxPos,                                  // position
        glm::vec2(INFO_PANEL_WIDTH, INFO_PANEL_HEIGHT), // size
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),       // color
        1000.1f                                  // z-index
    );
    
    // Create cursor position text (starting with a placeholder)
    cursorPositionText = std::make_shared<Rendering::Shapes::Text>(
        "Cursor: (0.0, 0.0)",                                      // initial text
        glm::vec2(INFO_PANEL_X + UI_PADDING, INFO_PANEL_Y + UI_PADDING), // position
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),                         // color
        1000.5f                                                    // z-index (increased to render on top)
    );
    
    // Create FPS text (starting with a placeholder)
    fpsText = std::make_shared<Rendering::Shapes::Text>(
        "FPS: 0.0",                                                        // initial text
        glm::vec2(INFO_PANEL_X + UI_PADDING, INFO_PANEL_Y + UI_PADDING + UI_LINE_HEIGHT), // position
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),                                 // color
        1000.5f                                                            // z-index (increased to render on top)
    );
    
    // Add all components to the UI layer
    uiLayer->addItem(infoPanelBackground);
    uiLayer->addItem(cursorPositionText);
    uiLayer->addItem(fpsText);
    
    // Update texts with initial values
    updateCursorText();
    updateFPSText();
}

void Interface::updateCursorText() {
    if (cursorPositionText) {
        std::stringstream cursorText;
        cursorText << "Cursor: (" << std::fixed << std::setprecision(1) 
                  << cursorWorldPosition.x << ", " << cursorWorldPosition.y << ")";
        cursorPositionText->setText(cursorText.str());
    }
}

void Interface::updateFPSText() {
    if (fpsText) {
        std::stringstream fpsTextStr;
        fpsTextStr << "FPS: " << std::fixed << std::setprecision(1) << currentFPS;
        fpsText->setText(fpsTextStr.str());
    }
}

void Interface::update(float deltaTime) {
   // No update needed - texts are updated when values change via setters
}

void Interface::render(VectorGraphics& graphics, const glm::mat4& projectionMatrix) {
    // Set projection for font renderer (still need this for text rendering)
    fontRenderer.setProjection(projectionMatrix);
    
    // Draw all UI components via their layers
    if (uiLayer) {
        uiLayer->render(graphics, glm::mat4(1.0f), projectionMatrix);
    }
}