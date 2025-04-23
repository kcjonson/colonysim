#include "WorldGen.h"
#include <iostream>
#include "../ScreenManager.h"
#include "../Game/World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <random>
#include "TerrainGenerator.h"

// Update constructor definition to accept Camera* and GLFWwindow*
WorldGenScreen::WorldGenScreen(Camera* camera, GLFWwindow* window)
    : lastCursorX(0.0f)
    , lastCursorY(0.0f)
    , sidebarWidth(300.0f)
    , worldWidth(256)
    , worldHeight(256)
    , waterLevel(0.4f)
    , worldGenerated(false) {
    
    // Generate a random seed
    std::random_device rd;
    seed = rd();
    
    // REMOVED: Pass nullptr for camera/window initially
    // Camera* camera = nullptr;
    // GLFWwindow* window = nullptr;

    // Create layers with different z-indices and pass pointers
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    controlsLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    previewLayer = std::make_shared<Rendering::Layer>(5.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
}

WorldGenScreen::~WorldGenScreen() {
}

bool WorldGenScreen::initialize() {
    // Define buttons
    buttons.clear();
    
    // Generate World button
    createButton("Generate World", 
                 glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),   // Green
                 glm::vec4(0.3f, 0.7f, 0.4f, 1.0f),   // Lighter green
                 [this]() {
                     generatedTerrainData.clear();
                     unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(std::to_string(seed));
                     WorldGen::TerrainGenerator::generateTerrain(
                         generatedTerrainData, worldWidth / 2, hashedSeed);
                     worldGenerated = true;
                     layoutUI();
                 });
    
    // Land button (go to gameplay)
    createButton("Land", 
                 glm::vec4(0.2f, 0.5f, 0.8f, 1.0f),   // Blue
                 glm::vec4(0.3f, 0.6f, 0.9f, 1.0f),   // Lighter blue
                 [this]() {
                     if (!worldGenerated) {
                         generatedTerrainData.clear();
                         unsigned int hashedSeed = WorldGen::TerrainGenerator::getHashedSeed(std::to_string(seed));
                         WorldGen::TerrainGenerator::generateTerrain(
                             generatedTerrainData, worldWidth / 2, hashedSeed);
                         worldGenerated = true;
                     }
                     if (screenManager->getWorld()) {
                         screenManager->getWorld()->setTerrainData(generatedTerrainData);
                     }
                     screenManager->switchScreen(ScreenType::Gameplay);
                 });
    
    // Back button
    createButton("Back", 
                 glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),   // Red
                 glm::vec4(0.9f, 0.3f, 0.3f, 1.0f),   // Lighter red
                 [this]() {
                     screenManager->switchScreen(ScreenType::MainMenu);
                 });
    
    // Set initial UI layout
    layoutUI();
    
    return true;
}

void WorldGenScreen::createButton(const std::string& text, const glm::vec4& color, const glm::vec4& hoverColor, const std::function<void()>& callback) {
    MenuButton button;
    button.text = text;
    button.color = color;
    button.hoverColor = hoverColor;
    button.isHovered = false;
    button.callback = callback;
    
    // The actual position and size will be set in layoutUI()
    buttons.push_back(button);
}

void WorldGenScreen::layoutUI() {
    // Get window size
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Clear all layers
    backgroundLayer->clearItems();
    controlsLayer->clearItems();
    buttonLayer->clearItems();
    previewLayer->clearItems();
    
    // Side bar takes up the left side of the screen
    sidebarWidth = 300.0f;
    
    // Create sidebar background
    auto sidebar = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(sidebarWidth, static_cast<float>(height)),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f)
        }),
        1.0f  // Z-index
    );
    backgroundLayer->addItem(sidebar);
    
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "World Generation",
        glm::vec2(40.0f, 70.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        15.0f  // Z-index
    );
    controlsLayer->addItem(titleText);
    
    // Create parameter labels and values
    float labelX = 40.0f;
    float valueX = 200.0f;
    float startY = 150.0f;
    float lineHeight = 30.0f;
    
    // Width parameter
    auto widthLabel = std::make_shared<Rendering::Shapes::Text>(
        "Width:",
        glm::vec2(labelX, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(widthLabel);
    
    auto widthValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldWidth),
        glm::vec2(valueX, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(widthValue);
    
    // Height parameter
    auto heightLabel = std::make_shared<Rendering::Shapes::Text>(
        "Height:",
        glm::vec2(labelX, startY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(heightLabel);
    
    auto heightValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldHeight),
        glm::vec2(valueX, startY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(heightValue);
    
    // Water level parameter
    auto waterLabel = std::make_shared<Rendering::Shapes::Text>(
        "Water Level:",
        glm::vec2(labelX, startY + 2 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(waterLabel);
    
    auto waterValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(waterLevel),
        glm::vec2(valueX, startY + 2 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(waterValue);
    
    // Seed parameter
    auto seedLabel = std::make_shared<Rendering::Shapes::Text>(
        "Seed:",
        glm::vec2(labelX, startY + 3 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(seedLabel);
    
    auto seedValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(seed),
        glm::vec2(valueX, startY + 3 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        12.0f  // Z-index
    );
    controlsLayer->addItem(seedValue);
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    float sidebarMargin = 40.0f;
    
    // Place buttons in sidebar
    for (size_t i = 0; i < buttons.size(); i++) {
        buttons[i].position.x = sidebarMargin;
        buttons[i].position.y = height - 150.0f - i * (buttonHeight + buttonSpacing);
        buttons[i].size.x = buttonWidth;
        buttons[i].size.y = buttonHeight;
        
        // Create button background
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
    
    // Add preview area message
    if (worldGenerated) {
        auto generatedMsg = std::make_shared<Rendering::Shapes::Text>(
            "World Generated",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 32.0f
            }),
            8.0f  // Z-index
        );
        previewLayer->addItem(generatedMsg);
        
        auto landMsg = std::make_shared<Rendering::Shapes::Text>(
            "Click 'Land' to begin",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f + 50.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            8.0f  // Z-index
        );
        previewLayer->addItem(landMsg);
    } else {
        auto customizeMsg = std::make_shared<Rendering::Shapes::Text>(
            "Use the controls to customize your world",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            8.0f  // Z-index
        );
        previewLayer->addItem(customizeMsg);
        
        auto generateMsg = std::make_shared<Rendering::Shapes::Text>(
            "Click 'Generate World' to create your world",
            glm::vec2(sidebarWidth + 20.0f, height / 2.0f + 40.0f),
            Rendering::Styles::Text({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 24.0f
            }),
            8.0f  // Z-index
        );
        previewLayer->addItem(generateMsg);
    }
}

void WorldGenScreen::update(float deltaTime) {
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

void WorldGenScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render all layers in order
    backgroundLayer->render(false);
    previewLayer->render(false);
    controlsLayer->render(false);
    buttonLayer->render(false);
}

void WorldGenScreen::handleInput() {
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

void WorldGenScreen::onResize(int width, int height) {
    // Re-layout UI when the window is resized
    layoutUI();
}

bool WorldGenScreen::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}