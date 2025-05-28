#include "Interface.h"
#include <sstream>
#include <iomanip>
#include <glm/glm.hpp>
#include "../../VectorGraphics.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Text.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include "../../Rendering/Styles/Shape.h"

const std::vector<std::string> Interface::GAME_STATE_PROPERTIES = {
    "world.totalTiles",
    "world.shownTiles",
    "world.loadedChunks",
    "world.tileMemKB",
    "world.shapeMemKB",
    "world.totalMemKB",
    "system.fps",
    "input.windowPos",
    "input.worldPos",
    "camera.position",
    "rend.vertices",
    "rend.indices"
};

// Constructor takes GameState, camera, and window
Interface::Interface(GameState& gameState, Camera* cam, GLFWwindow* win)
    : gameState(gameState),
      targetWindow(win), // Initialize targetWindow here
      uiLayer(std::make_shared<Rendering::Layer>(1000.0f, Rendering::ProjectionType::ScreenSpace, cam, win)),
      infoPanelBackground(nullptr) { // Initialize other members
    // Constructor body
}

bool Interface::initialize() {
    std::cout << "Initializing Interface..." << std::endl;
    // Initialize UI components here now that window/camera are available from constructor
    initializeUIComponents(); 
    std::cout << "Interface initialization complete" << std::endl;
    return true;
}

void Interface::initializeUIComponents() {
    // Calculate required height based on number of properties
    const float lineSpacing = UI_LINE_HEIGHT + 5.0f;
    const float panelHeight = UI_PADDING * 2 + GAME_STATE_PROPERTIES.size() * lineSpacing;
    
    // Create info panel background using top-left coordinates
    glm::vec2 topLeftPos(INFO_PANEL_X, INFO_PANEL_Y);    infoPanelBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = topLeftPos,
            .size = glm::vec2(INFO_PANEL_WIDTH, panelHeight),
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.0f, 0.0f, 0.0f, 0.6f),
                .cornerRadius = 5.0f
            }),
            .zIndex = 1000.1f
        }
    );
    uiLayer->addItem(infoPanelBackground);

    // Create text objects for each property
    propertyTexts.clear();
    for (size_t i = 0; i < GAME_STATE_PROPERTIES.size(); i++) {
        // Text positions start from top-left of panel plus padding
        glm::vec2 textPos(
            INFO_PANEL_X + UI_PADDING,
            INFO_PANEL_Y + UI_PADDING + (lineSpacing / 2) + (i * lineSpacing)
        );        auto text = std::make_shared<Rendering::Shapes::Text>(
            Rendering::Shapes::Text::Args{
                .text = GAME_STATE_PROPERTIES[i] + ": ...",
                .position = textPos,
                .style = Rendering::Shapes::Text::Styles({
                    .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)  // White text
                }),
                .zIndex = 1000.5f
            }
        );
        
        propertyTexts.push_back(text);
        uiLayer->addItem(text);
    }
}

void Interface::update(float deltaTime) {
    // Add defensive check to make sure gameState is valid
    try {
        for (size_t i = 0; i < GAME_STATE_PROPERTIES.size(); i++) {
            const auto& property = GAME_STATE_PROPERTIES[i];
            std::string value = "N/A"; // Default value

            try {
                // Check if reference is valid (to some extent)
                value = gameState.get(property);
            }
            catch (const std::exception& e) {
                std::cerr << "Error getting GameState property " << property << ": " << e.what() << std::endl;
                value = "Error";
            }

            // Update the text shape
            if (i < propertyTexts.size() && propertyTexts[i]) { // Add safety check
                propertyTexts[i]->setText(property + ": " + value);
            }
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error in Interface::update: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown fatal error in Interface::update" << std::endl;
    }
}

void Interface::render(bool batched) {
    // Render UI elements with screen-space projection
    if (uiLayer) {
        // Add UI elements to the batch
        uiLayer->render(false);
        
    }
}