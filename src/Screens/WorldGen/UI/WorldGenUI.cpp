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
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(0.0f, 0.0f),
            .size = glm::vec2(sidebarWidth, static_cast<float>(std::get<1>(windowSize))),
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.9f)
            }),
            .zIndex = 0.0f
        }
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
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                auto it = eventHandlers.find(UIEvent::GenerateWorld);
                if (it != eventHandlers.end()) {
                    it->second();
                }
            }
        }
    );
    sidebarLayer->addItem(generateWorldButton);

    auto landButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Land on World",
            .position = glm::vec2(40.0f, 420.0f),
            .size = glm::vec2(220.0f, 50.0f),
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                auto it = eventHandlers.find(UIEvent::GoToLand);
                if (it != eventHandlers.end()) {
                    it->second();
                }
            }
        }
    );
    sidebarLayer->addItem(landButton);

    auto cancelButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Back",
            .position = glm::vec2(40.0f, 490.0f),
            .size = glm::vec2(220.0f, 50.0f),
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                auto it = eventHandlers.find(UIEvent::Back);
                if (it != eventHandlers.end()) {
                    it->second();
                }
            }
        }
    );
    sidebarLayer->addItem(cancelButton);

    // Progress bar elements - positioned centrally below the globe
    float progressBarWidth = 300.0f; // Fixed width for the progress bar
    float progressBarHeight = 30.0f;
    float progressBarYOffset = 80.0f; // Distance above the status text
    
    // Initial positions - will be properly set in onResize
    float progressBarX = static_cast<float>(std::get<0>(windowSize)) / 2.0f - progressBarWidth / 2.0f;
    float progressBarY = static_cast<float>(std::get<1>(windowSize)) - 40.0f - progressBarYOffset;
      progressBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(progressBarX, progressBarY),
            .size = glm::vec2(progressBarWidth, progressBarHeight),
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                .cornerRadius = 5.0f
            }),
            .zIndex = 150.0f
        }
    );
    infoLayer->addItem(progressBackground);    progressFill = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(progressBarX, progressBarY),
            .size = glm::vec2(0.0f, progressBarHeight), // Initially 0 width
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .cornerRadius = 5.0f
            }),
            .zIndex = 151.0f
        }
    );
    infoLayer->addItem(progressFill);

    progressText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "0%",
            .position = glm::vec2(0.0f, 0.0f),
            .size = glm::vec2(progressBarWidth, progressBarHeight),
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
            .position = glm::vec2(0.0f, 0.0f),
            .size = glm::vec2(300.0f, 30.0f),
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
    radiusValue->setText(std::to_string(params.radius));
    massValue->setText(std::to_string(params.mass));
    waterValue->setText(std::to_string(params.waterAmount));
    seedValue->setText(std::to_string(params.seed));
}

void WorldGenUI::setProgress(float progress, const std::string& message) {
    currentProgress = progress;
    statusMessage = message;
    
    // If we're updating progress, ensure we're in the Generating state
    if (state != UIState::Generating) {
        setState(UIState::Generating);
    }
    
    // Update progress bar and text immediately (not just in next update call)
    if (progressFill && progressText) {
        float progressBarWidth = 300.0f;
        float progressBarHeight = 30.0f;
        progressFill->setSize(glm::vec2(progressBarWidth * currentProgress, progressBarHeight));
        
        int percentage = static_cast<int>(currentProgress * 100.0f);
        progressText->setText(std::to_string(percentage) + "%");
    }
    
    statusText->setText(statusMessage);
}

void WorldGenUI::onResize(int windowWidth, int windowHeight) {
    windowSize = std::make_tuple(windowWidth, windowHeight);

    // TODO: Subtract sidebar width from the window width
    statusText->setPosition(glm::vec2(windowWidth / 2.0f, windowHeight - 40.0f));
    progressBackground->setPosition(glm::vec2(windowWidth / 2.0f, windowHeight - 80.0f));
    progressFill->setPosition(glm::vec2(windowWidth / 2.0f, windowHeight - 80.0f));
    progressText->setPosition(glm::vec2(windowWidth / 2.0f, windowHeight - 80.0f));
}

void WorldGenUI::render() {
    // Render all layers in the correct order
    sidebarLayer->render();
    infoLayer->render();
}

void WorldGenUI::update(float /*deltaTime*/) {
    // Update progress bar and text regardless of state
    // (visibility is managed in layoutUI)
    float progressBarWidth = 300.0f;
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

}