#include "WorldGenUI.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "../../Camera.h"

namespace WorldGen {

WorldGenUI::WorldGenUI(Camera* camera, GLFWwindow* window)
    : sidebarWidth(300.0f)
    , camera(camera)
    , window(window)
    , state(UIState::ParameterSetup)
    , currentProgress(0.0f)
    , statusMessage("Ready to generate world") {
    
    // Create layers with different z-indices and pass pointers
    backgroundLayer = std::make_shared<Rendering::Layer>(-50.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    previewLayer = std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    sidebarLayer = std::make_shared<Rendering::Layer>(100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    controlsLayer = std::make_shared<Rendering::Layer>(150.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(200.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
}

WorldGenUI::~WorldGenUI() {
    // Nothing to destroy explicitly, smart pointers handle cleanup
}

bool WorldGenUI::initialize() {
    // Initialize buttons
    initializeButtons();
    return true;
}

void WorldGenUI::addEventListener(UIEvent event, UIEventCallback callback) {
    eventHandlers[event] = callback;
}

void WorldGenUI::initializeButtons() {
    // Clear existing buttons
    buttons.clear();
    
    // Generate World button
    createButton("Generate World", 
                glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),   // Green
                glm::vec4(0.3f, 0.7f, 0.4f, 1.0f),   // Lighter green
                [this]() {
                    // Trigger the generate world event if a handler is registered
                    auto it = eventHandlers.find(UIEvent::GenerateWorld);
                    if (it != eventHandlers.end()) {
                        it->second();
                    }
                });
    
    // Land button (go to gameplay)
    createButton("Land", 
                glm::vec4(0.2f, 0.5f, 0.8f, 1.0f),   // Blue
                glm::vec4(0.3f, 0.6f, 0.9f, 1.0f),   // Lighter blue
                [this]() {
                    // Trigger the go to land event if a handler is registered
                    auto it = eventHandlers.find(UIEvent::GoToLand);
                    if (it != eventHandlers.end()) {
                        it->second();
                    }
                });
    
    // Back button
    createButton("Back", 
                glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),   // Red
                glm::vec4(0.9f, 0.3f, 0.3f, 1.0f),   // Lighter red
                [this]() {
                    // Trigger the back event if a handler is registered
                    auto it = eventHandlers.find(UIEvent::Back);
                    if (it != eventHandlers.end()) {
                        it->second();
                    }
                });
}

void WorldGenUI::createButton(const std::string& text, const glm::vec4& color, 
                             const glm::vec4& hoverColor, const std::function<void()>& callback) {
    MenuButton button;
    button.text = text;
    button.color = color;
    button.hoverColor = hoverColor;
    button.isHovered = false;
    button.callback = callback;
    
    // The actual position and size will be set in layoutUI()
    buttons.push_back(button);
}

void WorldGenUI::setState(UIState newState) {
    if (state != newState) {
        state = newState;
        
        // When state changes, we might need to update UI elements
        // For now, we'll rely on the next layoutUI call to handle this
        std::cout << "UI State changed to: ";
        
        switch (state) {
            case UIState::ParameterSetup:
                std::cout << "Parameter Setup";
                break;
            case UIState::Generating:
                std::cout << "Generating";
                break;
            case UIState::Viewing:
                std::cout << "Viewing";
                break;
            case UIState::Saving:
                std::cout << "Saving";
                break;
            case UIState::Loading:
                std::cout << "Loading";
                break;
        }
        
        std::cout << std::endl;
    }
}

void WorldGenUI::handleButtonClicks(float mouseX, float mouseY, bool isPressed, bool wasPressed) {
    // Update hover state for all buttons
    for (auto& button : buttons) {
        button.isHovered = isPointInRect(mouseX, mouseY, 
            button.position.x, button.position.y, button.size.x, button.size.y);
    }
    
    // Handle click event only when button is first pressed
    if (isPressed && !wasPressed) {
        for (const auto& button : buttons) {
            if (button.isHovered && button.callback) {
                button.callback();
                break;
            }
        }
    }
}

void WorldGenUI::updateProgress(float progress, const std::string& message) {
    currentProgress = progress;
    statusMessage = message;
    
    // If we're updating progress, ensure we're in the Generating state
    if (state != UIState::Generating) {
        setState(UIState::Generating);
    }
    
    // Ideally we would update UI components immediately here
    // But for now we'll rely on the next layoutUI call
}

void WorldGenUI::layoutUI(int windowWidth, int windowHeight, int worldWidth, int worldHeight, 
                        float waterLevel, int seed, bool worldGenerated) {
    // Clear all layers
    backgroundLayer->clearItems();
    controlsLayer->clearItems();
    buttonLayer->clearItems();
    previewLayer->clearItems();
    sidebarLayer->clearItems();
    
    // Create sidebar background
    auto sidebar = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(sidebarWidth, static_cast<float>(windowHeight)),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f)
        }),
        100.0f  // Z-index matching sidebarLayer
    );
    sidebarLayer->addItem(sidebar);
    
    // Based on current state, setup appropriate UI
    if (state == UIState::Generating) {
        setupGeneratingUI(windowWidth, windowHeight);
    } else if (worldGenerated && state == UIState::Viewing) {
        setupViewingUI(windowWidth, windowHeight);
    } else {
        // Default to parameter setup UI
        setupParameterUI(windowWidth, windowHeight, worldWidth, worldHeight, waterLevel, seed);
    }
    
    // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    float sidebarMargin = 40.0f;
    
    // Place buttons in sidebar
    for (size_t i = 0; i < buttons.size(); i++) {
        buttons[i].position.x = sidebarMargin;
        buttons[i].position.y = windowHeight - 150.0f - i * (buttonHeight + buttonSpacing);
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
            200.0f  // Z-index matching buttonLayer
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
            200.0f  // Z-index matching buttonLayer
        );
        buttonLayer->addItem(buttons[i].label);
    }
    
    // Add status/progress text at bottom center for all states
    auto statusText = std::make_shared<Rendering::Shapes::Text>(
        statusMessage,
        glm::vec2(windowWidth / 2.0f, windowHeight - 40.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 20.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        250.0f // Above most UI
    );
    controlsLayer->addItem(statusText);
}

void WorldGenUI::setupParameterUI(int windowWidth, int windowHeight, int worldWidth, int worldHeight, 
                                 float waterLevel, int seed) {
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "World Generation",
        glm::vec2(40.0f, 70.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        150.0f  // Z-index matching controlsLayer
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
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(widthLabel);
    
    auto widthValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldWidth),
        glm::vec2(valueX, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
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
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(heightLabel);
    
    auto heightValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(worldHeight),
        glm::vec2(valueX, startY + lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
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
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(waterLabel);
    
    auto waterValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(waterLevel),
        glm::vec2(valueX, startY + 2 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
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
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(seedLabel);
    
    auto seedValue = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(seed),
        glm::vec2(valueX, startY + 3 * lineHeight),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(seedValue);
    
    // Preview area message
    auto customizeMsg = std::make_shared<Rendering::Shapes::Text>(
        "Use the controls to customize your world",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(customizeMsg);
    
    auto generateMsg = std::make_shared<Rendering::Shapes::Text>(
        "Click 'Generate World' to create your world",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f + 40.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(generateMsg);
}

void WorldGenUI::setupGeneratingUI(int windowWidth, int windowHeight) {
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "Generating World...",
        glm::vec2(40.0f, 70.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(titleText);
    
    // Progress bar background
    float progressBarWidth = sidebarWidth - 80.0f;
    float progressBarHeight = 30.0f;
    float progressBarY = 150.0f;
    
    auto progressBg = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(40.0f, progressBarY),
        glm::vec2(progressBarWidth, progressBarHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
            .cornerRadius = 5.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(progressBg);
    
    // Progress bar fill
    auto progressFill = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(40.0f, progressBarY),
        glm::vec2(progressBarWidth * currentProgress, progressBarHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
            .cornerRadius = 5.0f
        }),
        151.0f  // Z-index slightly above background
    );
    controlsLayer->addItem(progressFill);
    
    // Progress percentage
    int percentage = static_cast<int>(currentProgress * 100.0f);
    auto progressText = std::make_shared<Rendering::Shapes::Text>(
        std::to_string(percentage) + "%",
        glm::vec2(40.0f + progressBarWidth / 2.0f, progressBarY + progressBarHeight / 2.0f + 8.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 18.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        152.0f  // Z-index above fill
    );
    controlsLayer->addItem(progressText);
    
    // Preview area message
    auto generatingMsg = std::make_shared<Rendering::Shapes::Text>(
        "Generating World...",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(generatingMsg);
    
    auto pleaseWaitMsg = std::make_shared<Rendering::Shapes::Text>(
        "Please wait while your world is being created",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f + 50.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(pleaseWaitMsg);
}

void WorldGenUI::setupViewingUI(int windowWidth, int windowHeight) {
    // Create title
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        "World Generated",
        glm::vec2(40.0f, 70.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(titleText);
    
    // World stats section
    float startY = 150.0f;
    float lineHeight = 30.0f;
    
    auto statsTitle = std::make_shared<Rendering::Shapes::Text>(
        "World Statistics:",
        glm::vec2(40.0f, startY),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        150.0f  // Z-index matching controlsLayer
    );
    controlsLayer->addItem(statsTitle);
    
    // Preview area message
    auto generatedMsg = std::make_shared<Rendering::Shapes::Text>(
        "World Generated",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 32.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(generatedMsg);
    
    auto landMsg = std::make_shared<Rendering::Shapes::Text>(
        "Click 'Land' to begin",
        glm::vec2(sidebarWidth + 20.0f, windowHeight / 2.0f + 50.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 24.0f
        }),
        50.0f  // Z-index matching previewLayer
    );
    previewLayer->addItem(landMsg);
}

bool WorldGenUI::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
}

std::vector<std::shared_ptr<Rendering::Layer>> WorldGenUI::getAllLayers() const {
    return {
        backgroundLayer,
        previewLayer,
        sidebarLayer,
        controlsLayer,
        buttonLayer
    };
}

} // namespace WorldGen