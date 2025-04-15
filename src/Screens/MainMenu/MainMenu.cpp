#include "MainMenu.h"
#include "../ScreenManager.h"
#include "../../VectorGraphics.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

MainMenuScreen::MainMenuScreen()
    : lastCursorX(0.0f)
    , lastCursorY(0.0f) {
    
    // Create layers with different z-indices
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace);
    buttonLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace);
    titleLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace);
}

MainMenuScreen::~MainMenuScreen() {
}

bool MainMenuScreen::initialize() {
    // Define menu buttons
    buttons.clear();
    
    // Set window reference for all layers
    GLFWwindow* window = screenManager->getWindow();
    backgroundLayer->setWindow(window);
    buttonLayer->setWindow(window);
    titleLayer->setWindow(window);
    
    // Create the buttons
    createButton("Start Game", [this]() {
        screenManager->switchScreen(ScreenType::WorldGen);
    });
    
    createButton("Settings", [this]() {
        screenManager->switchScreen(ScreenType::Settings);
    });
    
    createButton("Developer", [this]() {
        screenManager->switchScreen(ScreenType::Developer);
    });
    
    // Layout buttons
    layoutButtons();
    
    // Create title
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "ColonySim",
        glm::vec2((width - 150.0f) / 2.0f, height * 0.2f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 48.0f
        }),
        25.0f  // Z-index
    );
    titleLayer->addItem(titleText);
    
    // Create menu box background
    float boxWidth = 300.0f;
    float boxHeight = 300.0f;
    float boxX = (width - boxWidth) / 2.0f;
    float boxY = (height - boxHeight) / 2.0f;
    
    auto menuBox = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(boxX, boxY),
        glm::vec2(boxWidth, boxHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f),
            .cornerRadius = 10.0f
        }),
        5.0f  // Z-index
    );
    backgroundLayer->addItem(menuBox);
    
    return true;
}

void MainMenuScreen::createButton(const std::string& text, const std::function<void()>& callback) {
    MenuButton button;
    button.text = text;
    button.color = glm::vec4(0.2f, 0.5f, 0.8f, 1.0f);     // Blue
    button.hoverColor = glm::vec4(0.3f, 0.6f, 0.9f, 1.0f); // Lighter blue
    button.isHovered = false;
    button.callback = callback;
    
    // The actual position and size will be set in layoutButtons()
    buttons.push_back(button);
}

void MainMenuScreen::layoutButtons() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Define menu box
    float boxWidth = 300.0f;
    float boxHeight = 300.0f;
    float boxX = (width - boxWidth) / 2.0f;
    float boxY = (height - boxHeight) / 2.0f;
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    
    // Place buttons in the menu box
    float startY = boxY + (boxHeight - buttons.size() * (buttonHeight + buttonSpacing) + buttonSpacing) / 2.0f;
    
    // Clear button layer first
    buttonLayer->clearItems();
    
    for (size_t i = 0; i < buttons.size(); i++) {
        // Update button position and size
        buttons[i].position.x = boxX + (boxWidth - buttonWidth) / 2.0f;
        buttons[i].position.y = startY + i * (buttonHeight + buttonSpacing);
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
            15.0f  // Z-index
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
            16.0f  // Z-index
        );
        buttonLayer->addItem(buttons[i].label);
    }
}

void MainMenuScreen::update(float deltaTime) {
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

void MainMenuScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render all layers in order
    backgroundLayer->render(false); // No camera transform
    buttonLayer->render(false);
    titleLayer->render(false);
}

void MainMenuScreen::handleInput() {
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
    
    // Check for ESC key to quit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void MainMenuScreen::onResize(int width, int height) {
    // Clear existing layers
    backgroundLayer->clearItems();
    buttonLayer->clearItems();
    titleLayer->clearItems();
    
    // Set window reference for all layers again after resize
    GLFWwindow* window = screenManager->getWindow();
    backgroundLayer->setWindow(window);
    buttonLayer->setWindow(window);
    titleLayer->setWindow(window);
    
    // Recreate title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "ColonySim",
        glm::vec2((width - 150.0f) / 2.0f, height * 0.2f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 48.0f
        }),
        25.0f  // Z-index
    );
    titleLayer->addItem(titleText);
    
    // Recreate menu box background
    float boxWidth = 300.0f;
    float boxHeight = 300.0f;
    float boxX = (width - boxWidth) / 2.0f;
    float boxY = (height - boxHeight) / 2.0f;
    
    auto menuBox = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(boxX, boxY),
        glm::vec2(boxWidth, boxHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f),
            .cornerRadius = 10.0f
        }),
        5.0f  // Z-index
    );
    backgroundLayer->addItem(menuBox);
    
    // Re-layout buttons
    layoutButtons();
}

bool MainMenuScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}