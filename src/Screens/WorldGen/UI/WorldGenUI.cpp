#include "WorldGenUI.h"
#include "Camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Rendering/Components/Button.h"

namespace WorldGen {

WorldGenUI::WorldGenUI(Camera* camera, GLFWwindow* window)
    : sidebarWidth(300.0f)
    , camera(camera)
    , window(window)
    , state(UIState::ParameterSetup)
    , currentProgress(0.0f)
    , statusMessage("Ready to generate world") {
    
    // Create layers with different z-indices and pass pointers
    sidebarLayer = std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    infoLayer = std::make_shared<Rendering::Layer>(150.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    windowSize = std::make_tuple(width, height);

        
    // Parameter labels and values
    float labelX = 40.0f;
    float valueX = 200.0f;
    float startY = 150.0f;
    float lineHeight = 30.0f;
    
    auto sidebarBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(sidebarWidth, static_cast<float>(std::get<1>(windowSize))),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f)
        }),
        0.0f
    );
    sidebarLayer->addItem(sidebarBackground);
    radiusLabel = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Size:",
            .position = glm::vec2(labelX, startY),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(radiusLabel);
      radiusValue = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0", // Will be updated in layoutUI
            .position = glm::vec2(valueX, startY),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(radiusValue);
    
        // Height parameter
    massLabel = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Mass:",
            .position = glm::vec2(labelX, startY + lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(massLabel);
      massValue = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0", // Will be updated in layoutUI
            .position = glm::vec2(valueX, startY + lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(massValue);
    
        // Water level parameter
    waterLabel = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Water Level:",
            .position = glm::vec2(labelX, startY + 2 * lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(waterLabel);
      waterValue = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0", // Will be updated in layoutUI
            .position = glm::vec2(valueX, startY + 2 * lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(waterValue);
    
        // Seed parameter
    seedLabel = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "Seed:",
            .position = glm::vec2(labelX, startY + 3 * lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(seedLabel);
    
    seedValue = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0", // Will be updated in layoutUI
            .position = glm::vec2(valueX, startY + 3 * lineHeight),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 150.0f
        }
    );
    sidebarLayer->addItem(seedValue);
    
    auto generateWorldButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Generate World",
            .position = glm::vec2(40.0f, 350.0f),
            .size = glm::vec2(220.0f, 50.0f),
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside, // Ensure BorderPosition is defined and accessible
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() { // The static_cast is often not needed for lambdas assigned to std::function
                std::cout << "Generate World button clicked" << std::endl;
                auto it = eventHandlers.find(UIEvent::GenerateWorld);
                if (it != eventHandlers.end()) {
                    it->second();
                }
            }
        }
    );
    sidebarLayer->addItem(generateWorldButton);


    // Progress bar elements
    float progressBarWidth = sidebarWidth - 80.0f;
    float progressBarHeight = 30.0f;
    float progressBarY = 150.0f;
    
    progressBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(40.0f, progressBarY),
        glm::vec2(progressBarWidth, progressBarHeight),
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
            .cornerRadius = 5.0f
        }),
        150.0f
    );
    infoLayer->addItem(progressBackground);
    
    progressFill = std::make_shared<Rendering::Shapes::Rectangle>(
        glm::vec2(40.0f, progressBarY),
        glm::vec2(0.0f, progressBarHeight), // Initially 0 width
        Rendering::Styles::Rectangle({
            .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
            .cornerRadius = 5.0f
        }),
        151.0f
    );
    infoLayer->addItem(progressFill);
      progressText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0%",
            .position = glm::vec2(40.0f + progressBarWidth / 2.0f, progressBarY + progressBarHeight / 2.0f + 8.0f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            .zIndex = 152.0f
        }
    );
    infoLayer->addItem(progressText);
      statusText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = statusMessage,
            .position = glm::vec2(0.0f, 0.0f), // Will be positioned in layoutUI
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f,
                .horizontalAlign = Rendering::TextAlign::Center,
                .verticalAlign = Rendering::TextAlign::Middle
            }),
            .zIndex = 250.0f
        }
    );
    infoLayer->addItem(statusText);
}

WorldGenUI::~WorldGenUI() {
    // Nothing to destroy explicitly, smart pointers handle cleanup
}

bool WorldGenUI::initialize() {
    
    std::cout << "Initializing WorldGenUI..." << std::endl;

    
    return true;
}

void WorldGenUI::addEventListener(UIEvent event, UIEventCallback callback) {
    eventHandlers[event] = callback;
}

// void WorldGenUI::initializeButtons() {
//     // Clear existing buttons
//     buttons.clear();
    
//     // Generate World button
//     createButton("Generate World", 
//                 glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),   // Green
//                 glm::vec4(0.3f, 0.7f, 0.4f, 1.0f),   // Lighter green
//                 [this]() {
//                     // Trigger the generate world event if a handler is registered
//                     auto it = eventHandlers.find(UIEvent::GenerateWorld);
//                     if (it != eventHandlers.end()) {
//                         it->second();
//                     }
//                 });
    
//     // Land button (go to gameplay)
//     createButton("Land", 
//                 glm::vec4(0.2f, 0.5f, 0.8f, 1.0f),   // Blue
//                 glm::vec4(0.3f, 0.6f, 0.9f, 1.0f),   // Lighter blue
//                 [this]() {
//                     // Trigger the go to land event if a handler is registered
//                     auto it = eventHandlers.find(UIEvent::GoToLand);
//                     if (it != eventHandlers.end()) {
//                         it->second();
//                     }
//                 });
    
//     // Back button
//     createButton("Back", 
//                 glm::vec4(0.8f, 0.2f, 0.2f, 1.0f),   // Red
//                 glm::vec4(0.9f, 0.3f, 0.3f, 1.0f),   // Lighter red
//                 [this]() {
//                     // Trigger the back event if a handler is registered
//                     auto it = eventHandlers.find(UIEvent::Back);
//                     if (it != eventHandlers.end()) {
//                         it->second();
//                     }
//                 });
// }

// void WorldGenUI::createButton(const std::string& text, const glm::vec4& color, 
//                              const glm::vec4& hoverColor, const std::function<void()>& callback) {
//     MenuButton button;
//     button.text = text;
//     button.color = color;
//     button.hoverColor = hoverColor;
//     button.isHovered = false;
//     button.callback = callback;
    
//     // The actual position and size will be set in layoutUI()
//     buttons.push_back(button);
// }

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

void WorldGenUI::setPlanetParameters(const PlanetParameters& params) {

    // Update UI elements with new parameters

    radiusValue->setText(std::to_string(params.radius));
    massValue->setText(std::to_string(params.mass));
    waterValue->setText(std::to_string(params.waterAmount));
    seedValue->setText(std::to_string(params.seed));
 
}

// void WorldGenUI::handleButtonClicks(float mouseX, float mouseY, bool isPressed, bool wasPressed) {
//     // Update hover state for all buttons
//     for (auto& button : buttons) {
//         button.isHovered = isPointInRect(mouseX, mouseY, 
//             button.position.x, button.position.y, button.size.x, button.size.y);
//     }
    
//     // Handle click event only when button is first pressed
//     if (isPressed && !wasPressed) {
//         for (const auto& button : buttons) {
//             if (button.isHovered && button.callback) {
//                 button.callback();
//                 break;
//             }
//         }
//     }
// }

void WorldGenUI::setProgress(float progress, const std::string& message) {
    currentProgress = progress;
    statusMessage = message;
    

    // If we're updating progress, ensure we're in the Generating state
    if (state != UIState::Generating) {
        setState(UIState::Generating);
    }
    
    // Update progress bar and text immediately (not just in next update call)
    if (progressFill && progressText) {
        float progressBarWidth = sidebarWidth - 80.0f;
        float progressBarHeight = 30.0f;
        progressFill->setSize(glm::vec2(progressBarWidth * currentProgress, progressBarHeight));
        
        int percentage = static_cast<int>(currentProgress * 100.0f);
        progressText->setText(std::to_string(percentage) + "%");
    }
    

    statusText->setText(statusMessage);
}

void WorldGenUI::onResize(int windowWidth, int windowHeight) {

    windowSize = std::make_tuple(windowWidth, windowHeight);
    // Create sidebar background (this is created on demand rather than stored as a member)


      // Button dimensions
    float buttonWidth = 220.0f;
    float buttonHeight = 50.0f;
    float buttonSpacing = 20.0f;
    float sidebarMargin = 40.0f;
    

    
    // // Place buttons in sidebar
    // for (size_t i = 0; i < buttons.size(); i++) {
    //     buttons[i].position.x = sidebarMargin;
    //     buttons[i].position.y = windowHeight - 150.0f - i * (buttonHeight + buttonSpacing);
    //     buttons[i].size.x = buttonWidth;
    //     buttons[i].size.y = buttonHeight;
        
    //     // Create button background
    //     buttons[i].background = std::make_shared<Rendering::Shapes::Rectangle>(
    //         buttons[i].position,
    //         buttons[i].size,
    //         Rendering::Styles::Rectangle({
    //             .color = buttons[i].isHovered ? buttons[i].hoverColor : buttons[i].color,
    //             .cornerRadius = 5.0f
    //         }),
    //         200.0f  // Z-index matching buttonLayer
    //     );
    //     buttonLayer->addItem(buttons[i].background);
        
    //     // Create button text
    //     float textY = buttons[i].position.y + buttons[i].size.y / 2.0f + 8.0f; // Center text vertically
    //     buttons[i].label = std::make_shared<Rendering::Shapes::Text>(
    //         buttons[i].text,
    //         glm::vec2(buttons[i].position.x + buttons[i].size.x / 2.0f, textY),
    //         Rendering::Styles::Text({
    //             .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    //             .fontSize = 1.0f,
    //             .horizontalAlign = Rendering::TextAlign::Center,
    //             .verticalAlign = Rendering::TextAlign::Middle
    //         }),
    //         200.0f  // Z-index matching buttonLayer
    //     );
    //     buttonLayer->addItem(buttons[i].label);
    // }
    
    statusText->setPosition(glm::vec2(windowWidth / 2.0f, windowHeight - 40.0f));

}

void WorldGenUI::render() {
    // Render all layers in the correct order
    sidebarLayer->render();
    infoLayer->render();
}

void WorldGenUI::update(float /*deltaTime*/) {
    // Update progress bar and text regardless of state
    // (visibility is managed in layoutUI)
    float progressBarWidth = sidebarWidth - 80.0f;
    float progressBarHeight = 30.0f;
    

    progressFill->setSize(glm::vec2(progressBarWidth * currentProgress, progressBarHeight));

    int percentage = static_cast<int>(currentProgress * 100.0f);
    progressText->setText(std::to_string(percentage) + "%");


    statusText->setText(statusMessage);

}

void WorldGenUI::handleInput(float deltaTime) {
    sidebarLayer->handleInput(deltaTime);
    infoLayer->handleInput(deltaTime);
}

// bool WorldGenUI::isPointInRect(float px, float py, float rx, float ry, float rw, float rh) {
//     return px >= rx && px <= rx + rw && py >= ry && py <= ry + rh;
// }

}