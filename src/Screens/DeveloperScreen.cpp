#include "DeveloperScreen.h"
#include "../ScreenManager.h"
#include "../VectorGraphics.h"
#include "../Examples.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

DeveloperScreen::DeveloperScreen()
    : lastCursorX(0.0f)
    , lastCursorY(0.0f) {
    
    // Create layers with different z-indices
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace);
    buttonLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace);
    titleLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace);
}

DeveloperScreen::~DeveloperScreen() {
}

bool DeveloperScreen::initialize() {
    // Define buttons
    buttons.clear();
    
    // Set window reference for all layers
    GLFWwindow* window = screenManager->getWindow();
    backgroundLayer->setWindow(window);
    buttonLayer->setWindow(window);
    titleLayer->setWindow(window);
    
    // Set window reference for Examples if available
    if (screenManager->getExamples()) {
        screenManager->getExamples()->setWindow(window);
    }
    
    // Back button
    MenuButton backButton;
    backButton.text = "Back to Menu";
    backButton.color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // Red
    backButton.hoverColor = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f);
    backButton.isHovered = false;
    backButton.callback = [this]() {
        screenManager->switchScreen(ScreenType::MainMenu);
    };
    
    buttons.push_back(backButton);
    
    // Layout UI elements
    layoutUI();
    
    return true;
}

void DeveloperScreen::layoutUI() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Set window reference for all layers
    backgroundLayer->setWindow(window);
    buttonLayer->setWindow(window);
    titleLayer->setWindow(window);
    
    // Clear all layers
    backgroundLayer->clearItems();
    buttonLayer->clearItems();
    titleLayer->clearItems();
    
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "Developer Mode",
        glm::vec2(width / 2.0f, 80.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 48.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        15.0f  // Z-index
    );
    titleLayer->addItem(titleText);
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    
    // Position the back button at the bottom right
    buttons[0].position.x = width - buttonWidth - 20.0f;
    buttons[0].position.y = height - buttonHeight - 20.0f;
    buttons[0].size.x = buttonWidth;
    buttons[0].size.y = buttonHeight;
    
    // Create button background rectangle
    buttons[0].background = std::make_shared<Rendering::Shapes::Rectangle>(
        buttons[0].position,
        buttons[0].size,
        Rendering::Styles::Rectangle({
            .color = buttons[0].isHovered ? buttons[0].hoverColor : buttons[0].color,
            .cornerRadius = 5.0f
        }),
        25.0f  // Z-index
    );
    buttonLayer->addItem(buttons[0].background);
    
    // Create button text
    float textY = buttons[0].position.y + buttons[0].size.y / 2.0f + 8.0f; // Center text vertically
    buttons[0].label = std::make_shared<Rendering::Shapes::Text>(
        buttons[0].text,
        glm::vec2(buttons[0].position.x + buttons[0].size.x / 2.0f, textY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        26.0f  // Z-index
    );
    buttonLayer->addItem(buttons[0].label);
}

void DeveloperScreen::update(float deltaTime) {
    // Update button hover states
    for (size_t i = 0; i < buttons.size(); i++) {
        // Update button color based on hover state
        if (buttons[i].background) {
            // Create a new style with the updated color
            Rendering::Styles::Rectangle newStyle = buttons[i].background->getStyle();
            newStyle.color = buttons[i].isHovered ? buttons[i].hoverColor : buttons[i].color;
            buttons[i].background->setStyle(newStyle);
        }
    }
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

void DeveloperScreen::handleInput() {
    GLFWwindow* window = screenManager->getWindow();
    
    // Get cursor position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    lastCursorX = static_cast<float>(xpos);
    lastCursorY = static_cast<float>(ypos);
    
    // Check for button hover
    for (auto& button : buttons) {
        button.isHovered = isPointInRect(
            lastCursorX, lastCursorY,
            button.position.x, button.position.y,
            button.size.x, button.size.y
        );
    }
    
    // Check for button click
    static bool wasPressed = false;
    bool isPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    if (isPressed && !wasPressed) {
        for (const auto& button : buttons) {
            if (button.isHovered && button.callback) {
                button.callback();
                break;
            }
        }
    }
    
    wasPressed = isPressed;
    
    // Check for ESC key to go back to main menu
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
}

void DeveloperScreen::onResize(int width, int height) {
    // Re-layout UI when the window is resized
    layoutUI();
    
    // Update Examples window reference if needed
    GLFWwindow* window = screenManager->getWindow();
    if (screenManager->getExamples()) {
        screenManager->getExamples()->setWindow(window);
    }
}

bool DeveloperScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}