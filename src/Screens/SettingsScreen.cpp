#include "SettingsScreen.h"
#include "../ScreenManager.h"
#include "../VectorGraphics.h"
#include "../ConfigManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

SettingsScreen::SettingsScreen()
    : lastCursorX(0.0f)
    , lastCursorY(0.0f) {
    
    // Create layers with different z-indices
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace);
    controlsLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace);
    buttonLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace);
}

SettingsScreen::~SettingsScreen() {
}

bool SettingsScreen::initialize() {
    // Define buttons
    buttons.clear();
    
    // Set window reference for all layers
    GLFWwindow* window = screenManager->getWindow();
    backgroundLayer->setWindow(window);
    controlsLayer->setWindow(window);
    buttonLayer->setWindow(window);
    
    // Back button
    MenuButton backButton;
    backButton.text = "Back";
    backButton.color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);  // Red
    backButton.hoverColor = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f);
    backButton.isHovered = false;
    backButton.callback = [this]() {
        screenManager->switchScreen(ScreenType::MainMenu);
    };
    
    // Save Settings button
    MenuButton saveButton;
    saveButton.text = "Save Settings";
    saveButton.color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f);  // Green
    saveButton.hoverColor = glm::vec4(0.3f, 0.7f, 0.4f, 1.0f);
    saveButton.isHovered = false;
    saveButton.callback = [this]() {  // Fixed: using saveButton.callback instead of backButton.callback
        // In the future, we would save actual settings here
        // For now, just provide visual feedback
        std::cout << "Settings would be saved here" << std::endl;
    };
    
    buttons.push_back(saveButton);
    buttons.push_back(backButton);
    
    // Layout UI elements
    layoutUI();
    
    return true;
}

void SettingsScreen::layoutUI() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Set window reference for all layers
    backgroundLayer->setWindow(window);
    controlsLayer->setWindow(window);
    buttonLayer->setWindow(window);
    
    // Clear all layers
    backgroundLayer->clearItems();
    controlsLayer->clearItems();
    buttonLayer->clearItems();
    
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "Settings",
        glm::vec2(width / 2.0f, 80.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 48.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        25.0f  // Z-index
    );
    controlsLayer->addItem(titleText);
    
    // Create settings panel background
    float panelWidth = width * 0.8f;
    float panelHeight = height * 0.7f;
    float panelX = (width - panelWidth) / 2.0f;
    float panelY = 130.0f;
    
    auto panelBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(panelX, panelY),
        glm::vec2(panelWidth, panelHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f),
            .cornerRadius = 10.0f
        }),
        5.0f  // Z-index
    );
    backgroundLayer->addItem(panelBackground);
    
    // Add settings options (placeholders for now)
    float settingsX = panelX + 50.0f;
    float settingsY = panelY + 50.0f;
    float lineHeight = 40.0f;
    
    auto resolutionText = std::make_shared<Rendering::Shapes::Text>(
        "Resolution: 1280x720",
        glm::vec2(settingsX, settingsY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(resolutionText);
    
    auto fullscreenText = std::make_shared<Rendering::Shapes::Text>(
        "Fullscreen: Off",
        glm::vec2(settingsX, settingsY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(fullscreenText);
    
    auto vsyncText = std::make_shared<Rendering::Shapes::Text>(
        "VSync: On",
        glm::vec2(settingsX, settingsY + lineHeight * 2),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(vsyncText);
    
    auto soundText = std::make_shared<Rendering::Shapes::Text>(
        "Sound Volume: 80%",
        glm::vec2(settingsX, settingsY + lineHeight * 3),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(soundText);
    
    auto musicText = std::make_shared<Rendering::Shapes::Text>(
        "Music Volume: 50%",
        glm::vec2(settingsX, settingsY + lineHeight * 4),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(musicText);
    
    // Add "settings still in development" message
    auto devMsg = std::make_shared<Rendering::Shapes::Text>(
        "Settings functionality is still in development",
        glm::vec2(width / 2.0f, height - 40.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f),
            .fontSize = 18.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(devMsg);
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    
    // Center buttons horizontally and position at bottom of screen
    float startY = height - 100.0f;
    float startX = (width - buttonWidth) / 2.0f;
    
    for (size_t i = 0; i < buttons.size(); i++) {
        buttons[i].position.x = startX;
        buttons[i].position.y = startY - i * (buttonHeight + buttonSpacing);
        buttons[i].size.x = buttonWidth;
        buttons[i].size.y = buttonHeight;
        
        // Create button background rectangle
        buttons[i].background = std::make_shared<Rendering::Shapes::Rectangle>(
            buttons[i].position,
            buttons[i].size,
            Rendering::Styles::Rectangle({
                .color = buttons[i].isHovered ? buttons[i].hoverColor : buttons[i].color,
                .cornerRadius = 5.0f
            }),
            25.0f  // Z-index
        );
        buttonLayer->addItem(buttons[i].background);
        
        // Create button text
        float textY = buttons[i].position.y + buttons[i].size.y / 2.0f + 8.0f; // Center text vertically
        buttons[i].label = std::make_shared<Rendering::Shapes::Text>(
            buttons[i].text,
            glm::vec2(buttons[i].position.x + buttons[i].size.x / 2.0f, textY),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            26.0f  // Z-index
        );
        buttonLayer->addItem(buttons[i].label);
    }
}

void SettingsScreen::update(float deltaTime) {
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

void SettingsScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render all layers in order
    backgroundLayer->render(false);
    controlsLayer->render(false);
    buttonLayer->render(false);
}

void SettingsScreen::handleInput() {
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

void SettingsScreen::onResize(int width, int height) {
    // Re-layout UI when the window is resized
    layoutUI();
}

bool SettingsScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}