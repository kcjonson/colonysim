#include "Settings.h"
#include "../ScreenManager.h"
#include "../../VectorGraphics.h"
#include "../../ConfigManager.h"
#include "../../CoordinateSystem.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Update constructor definition to accept Camera* and GLFWwindow*
SettingsScreen::SettingsScreen(Camera* camera, GLFWwindow* window)
    : lastCursorX(0.0f)
    , lastCursorY(0.0f) {
    
    // REMOVED: Pass nullptr for camera/window initially
    // Camera* camera = nullptr;
    // GLFWwindow* window = nullptr;

    // Create layers with different z-indices and pass pointers
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    controlsLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    // Back button using new Button component
    backButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::Button::Args{
            .label = "Back",
            .type = Rendering::Components::Button::Type::Primary,
            .onClick = [this]() {
                screenManager->switchScreen(ScreenType::MainMenu);
            }
        }
    );
    buttonLayer->addItem(backButton);
    
    // Save Settings button using new Button component
    saveButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::Button::Args{
            .label = "Save Settings",
            .type = Rendering::Components::Button::Type::Primary,
            .onClick = [this]() {
                // In the future, we would save actual settings here
                // For now, just provide visual feedback
                std::cout << "Settings would be saved here" << std::endl;
            }
        }
    );
    buttonLayer->addItem(saveButton);
}

SettingsScreen::~SettingsScreen() {
}

bool SettingsScreen::initialize() {
    // REMOVED: Define buttons
    // buttons.clear();
    
    // REMOVED: Set window reference for all layers
    // GLFWwindow* window = screenManager->getWindow();
    // backgroundLayer->setWindow(window);
    // controlsLayer->setWindow(window);
    // buttonLayer->setWindow(window);
    
    // Layout UI elements (addItem calls will propagate pointers)
    layoutUI();
    
    return true;
}

void SettingsScreen::layoutUI() {
    // Use coordinate system for consistent layout
    auto& coordSys = CoordinateSystem::getInstance();
    auto windowSize = coordSys.getWindowSize();
    float width = windowSize.x;
    float height = windowSize.y;
    
    // REMOVED: Set window reference for all layers
    // backgroundLayer->setWindow(window);
    // controlsLayer->setWindow(window);
    // buttonLayer->setWindow(window);
    
    // Clear all layers
    backgroundLayer->clearItems();
    controlsLayer->clearItems();
    buttonLayer->clearItems();
      // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Settings",
            .position = glm::vec2(width / 2.0f, 80.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            .zIndex = 25.0f  // Z-index
        }
    );
    controlsLayer->addItem(titleText);
    
    // Create settings panel background
    float panelWidth = width * 0.8f;
    float panelHeight = height * 0.7f;
    float panelX = (width - panelWidth) / 2.0f;
    float panelY = 130.0f;
    
    auto panelBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(panelX, panelY),
            .size = glm::vec2(panelWidth, panelHeight),
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f),
                .cornerRadius = 10.0f
            }),
            .zIndex = 5.0f  // Z-index
        }
    );
    backgroundLayer->addItem(panelBackground);
    
    // Add settings options (placeholders for now)
    float settingsX = panelX + 50.0f;
    float settingsY = panelY + 50.0f;
    float lineHeight = 40.0f;
      auto resolutionText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Resolution: 1280x720",
            .position = glm::vec2(settingsX, settingsY),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(resolutionText);
      auto fullscreenText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Fullscreen: Off",
            .position = glm::vec2(settingsX, settingsY + lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(fullscreenText);
      auto vsyncText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "VSync: On",
            .position = glm::vec2(settingsX, settingsY + lineHeight * 2),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(vsyncText);
      auto soundText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Sound Volume: 80%",
            .position = glm::vec2(settingsX, settingsY + lineHeight * 3),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(soundText);
      auto musicText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Music Volume: 50%",
            .position = glm::vec2(settingsX, settingsY + lineHeight * 4),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(musicText);
      // Add "settings still in development" message
    auto devMsg = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Settings functionality is still in development",
            .position = glm::vec2(width / 2.0f, height - 40.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
                .fontSize = 1.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            .zIndex = 15.0f  // Z-index
        }
    );
    controlsLayer->addItem(devMsg);
      // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    
    // Center buttons horizontally and position at bottom of screen
    float startY = height - 100.0f;
    float startX = (width - buttonWidth) / 2.0f;
    
    // Position the save button
    saveButton->setPosition(glm::vec2(startX, startY - 0 * (buttonHeight + buttonSpacing)));
    saveButton->setSize(glm::vec2(buttonWidth, buttonHeight));
    
    // Position the back button below save button
    backButton->setPosition(glm::vec2(startX, startY - 1 * (buttonHeight + buttonSpacing)));
    backButton->setSize(glm::vec2(buttonWidth, buttonHeight));
}

void SettingsScreen::update(float deltaTime) {
    // No need to update button hover states manually anymore
    // The Button component handles that internally
}

void SettingsScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render all layers in order
    backgroundLayer->render(false);
    controlsLayer->render(false);
    buttonLayer->render(false);
}

void SettingsScreen::handleInput(float deltaTime) {
    GLFWwindow* window = screenManager->getWindow();
    
    // Get cursor position for any custom input handling
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    lastCursorX = static_cast<float>(xpos);
    lastCursorY = static_cast<float>(ypos);
    
    // Let the button layer handle input for buttons
    buttonLayer->handleInput(deltaTime);
    
    // Check for ESC key to go back to main menu
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
}

void SettingsScreen::onResize(int width, int height) {
    // REMOVED: Set window reference for all layers again after resize
    // GLFWwindow* window = screenManager->getWindow();
    // backgroundLayer->setWindow(window);
    // controlsLayer->setWindow(window);
    // buttonLayer->setWindow(window);

    // Re-layout UI which clears and re-adds items
    layoutUI();
}

bool SettingsScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}