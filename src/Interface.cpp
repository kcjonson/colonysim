#include "Interface.h"
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include "VectorGraphics.h"
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Text.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "Rendering/Styles/Shape.h"

const std::vector<std::string> Interface::GAME_STATE_PROPERTIES = {
    "world.tileCount",
    "world.totalShapes",
    "world.tileMemoryKB",
    "world.shapeMemoryKB",
    "world.totalMemoryKB",
    "system.fps",
    "input.cursorWindowPosition",
    "input.cursorWorldPosition",
    "camera.position"
};

Interface::Interface(GameState& gameState)
    : gameState(gameState),
      uiLayer(nullptr),
      targetWindow(nullptr),
      infoPanelBackground(nullptr) {
    // Create UI layer with high z-index and ScreenSpace projection
    uiLayer = std::make_shared<Rendering::Layer>(1000.0f, Rendering::ProjectionType::ScreenSpace);
    // Window and renderer will be set in initializeGraphics
}

bool Interface::initialize() {
    std::cout << "Initializing Interface..." << std::endl;
    std::cout << "Interface initialization complete" << std::endl;
    return true;
}

// TODO: move to the other initialize function
bool Interface::initializeGraphics(GLFWwindow* window) {
    // std::cout << "Interface::initializeGraphics - Setting window and initializing UI components" << std::endl;
    
    targetWindow = window;
    // Set window on the UI layer for screen space projection calculation
    uiLayer->setWindow(targetWindow);
    initializeUIComponents();
    return true;
}

void Interface::initializeUIComponents() {
    // Calculate required height based on number of properties
    const float lineSpacing = UI_LINE_HEIGHT + 5.0f;
    const float panelHeight = UI_PADDING * 2 + GAME_STATE_PROPERTIES.size() * lineSpacing;
    
    // Create info panel background using top-left coordinates
    glm::vec2 topLeftPos(INFO_PANEL_X, INFO_PANEL_Y);
    infoPanelBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        topLeftPos,
        glm::vec2(INFO_PANEL_WIDTH, panelHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f)  // Black background
        }),
        1000.1f
    );
    uiLayer->addItem(infoPanelBackground);

    // Create text objects for each property
    propertyTexts.clear();
    for (size_t i = 0; i < GAME_STATE_PROPERTIES.size(); i++) {
        // Text positions start from top-left of panel plus padding
        glm::vec2 textPos(
            INFO_PANEL_X + UI_PADDING,
            INFO_PANEL_Y + UI_PADDING + i * lineSpacing
        );

        auto text = std::make_shared<Rendering::Shapes::Text>(
            GAME_STATE_PROPERTIES[i] + ": ...",
            textPos,
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // White text
            }),
            1000.5f
        );
        
        propertyTexts.push_back(text);
        uiLayer->addItem(text);
    }
}

void Interface::update(float deltaTime) {
    for (size_t i = 0; i < GAME_STATE_PROPERTIES.size(); i++) {
        const auto& property = GAME_STATE_PROPERTIES[i];
        propertyTexts[i]->setText(property + ": " + gameState.get(property));
    }
}

void Interface::render(VectorGraphics& graphics) {
    // Render UI elements with screen-space projection
    if (uiLayer) {
        // Add UI elements to the batch
        uiLayer->render(graphics);
        
        // Get the appropriate view and projection matrices for screen space
        glm::mat4 viewMatrix = uiLayer->getViewMatrix(); // Identity for screen space
        glm::mat4 projectionMatrix = uiLayer->getProjectionMatrix();
        
        // Finalize the UI batch with screen-space projection
        graphics.render(viewMatrix, projectionMatrix);
    }
}

void Interface::setRenderer(Renderer* r) {
    if (uiLayer) {
        uiLayer->setRenderer(r);
    }
}