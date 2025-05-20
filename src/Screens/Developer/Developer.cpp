#include "Developer.h"
#include "../ScreenManager.h"
#include "../../VectorGraphics.h"
#include "Examples.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Update constructor definition to accept Camera* and GLFWwindow*
DeveloperScreen::DeveloperScreen(Camera* camera, GLFWwindow* window)
    : lastCursorX(0.0f)
    , lastCursorY(0.0f) {
    
    // Create layers with different z-indices and pass window/camera
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    titleLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    // Create back button using the new Button component
    backButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::Button::Args{
            .label = "Back to Menu",
            .type = Rendering::Components::Button::Type::Primary,
            .onClick = [this]() {
                screenManager->switchScreen(ScreenType::MainMenu);
            }
        }
    );
    buttonLayer->addItem(backButton);
}

DeveloperScreen::~DeveloperScreen() {
}

bool DeveloperScreen::initialize() {
    // Get window reference (camera not needed for ScreenSpace layers here)
    GLFWwindow* window = screenManager->getWindow();
    // REMOVED: backgroundLayer->setWindow(window);
    // REMOVED: buttonLayer->setWindow(window);
    // REMOVED: titleLayer->setWindow(window);

    // Update layers with the correct window pointer if they were created with nullptr
    // This assumes Layer constructor or addItem handles setting these if passed non-null
    // If the constructor properly sets them, this might not be strictly needed,
    // but it ensures they have the correct window pointer from the manager.
    // Re-creating layers here might be cleaner if ScreenManager isn't available in constructor.
    // For now, let's assume the pointers passed via addItem or constructor are sufficient.

    // Set window reference for Examples if available
    // Examples class needs similar modification
    if (screenManager->getExamples()) {
        // screenManager->getExamples()->setWindow(window); // This needs to be removed/refactored in Examples.cpp
    }
    
    // Layout UI elements
    layoutUI();
    
    return true;
}

void DeveloperScreen::layoutUI() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Clear all layers
    backgroundLayer->clearItems();
    titleLayer->clearItems();
    
    // We don't need to clear buttonLayer as we're managing the buttons directly
    
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Developer Mode",
            .position = glm::vec2(width / 2.0f, 80.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            .zIndex = 15.0f
        }
    );
    titleLayer->addItem(titleText);
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    
    // Position the back button at the bottom right
    backButton->setPosition(glm::vec2(width - buttonWidth - 20.0f, height - buttonHeight - 20.0f));
    backButton->setSize(glm::vec2(buttonWidth, buttonHeight));
}

void DeveloperScreen::update(float deltaTime) {
    // No need to update button hover states manually anymore
    // The Button component handles that internally
}

void DeveloperScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render examples if available
    Examples* examples = screenManager->getExamples();
    if (examples) {
        examples->render();
    }
    
    // Render all UI layers on top
    backgroundLayer->render(false);
    titleLayer->render(false);
    buttonLayer->render(false);
}

void DeveloperScreen::handleInput(float deltaTime) {
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

void DeveloperScreen::onResize(int width, int height) {
    // Re-layout UI when the window is resized
    layoutUI();
    
    // Update Examples window reference if needed
    // This needs refactoring in Examples class
    GLFWwindow* window = screenManager->getWindow();
    if (screenManager->getExamples()) {
        // screenManager->getExamples()->setWindow(window); // Remove/refactor
    }
}

bool DeveloperScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}