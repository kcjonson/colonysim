#include "MainMenu.h"
#include "../ScreenManager.h"
#include "../../VectorGraphics.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Rendering/Components/Button.h"


// Update constructor definition to accept Camera* and GLFWwindow*
MainMenuScreen::MainMenuScreen(Camera* camera, GLFWwindow* window) {
    
    // Create layers with different z-indices and pass pointers
    backgroundLayer = std::make_shared<Rendering::Layer>(0.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    buttonLayer = std::make_shared<Rendering::Layer>(10.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
    titleLayer = std::make_shared<Rendering::Layer>(20.0f, Rendering::ProjectionType::ScreenSpace, camera, window);


    // Create title
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    auto titleText = std::make_shared<Rendering::Shapes::Text>(
        Rendering::Shapes::Text::Args{
            .text = "ColonySim",
            .position = glm::vec2((width - 150.0f) / 2.0f, height * 0.2f),
            .style = Rendering::Shapes::Text::Styles({
                .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                .fontSize = 1.0f
            }),
            .zIndex = 25.0f
        }
    );
    titleLayer->addItem(titleText);
    
    // Create menu box background
    float boxWidth = 300.0f;
    float boxHeight = 300.0f;
    float boxX = (width - boxWidth) / 2.0f;
    float boxY = (height - boxHeight) / 2.0f;
    

    menuBackground = std::make_shared<Rendering::Shapes::Rectangle>(
        Rendering::Shapes::Rectangle::Args{
            .position = glm::vec2(boxX, boxY),
            .size = glm::vec2(boxWidth, boxHeight),
            .style = Rendering::Shapes::Rectangle::Styles({
                .color = glm::vec4(0.1f, 0.1f, 0.1f, 0.8f),
                .cornerRadius = 10.0f
            }),
            .zIndex = 5.0f
        }
    );

    backgroundLayer->addItem(menuBackground);

    newColonyButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "New Colony",
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                std::cout << "New Colony button clicked!" << std::endl;
                screenManager->switchScreen(ScreenType::WorldGen);
            }
        }
    );
    buttonLayer->addItem(newColonyButton);

    loadColonyButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Load Colony",
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                std::cout << "Load Colony button clicked!" << std::endl;
            }
        }
    );
    buttonLayer->addItem(loadColonyButton);

    settingsButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Settings",
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                std::cout << "Settings button clicked!" << std::endl;
                screenManager->switchScreen(ScreenType::Settings);
            }
        }
    );
    buttonLayer->addItem(settingsButton);


    developerButton = std::make_shared<Rendering::Components::Button>(
        Rendering::Components::ButtonArgs{
            .label = "Developer",
            .style = Rendering::Styles::Button({
                .color = glm::vec4(0.2f, 0.6f, 0.3f, 1.0f),
                .borderColor = glm::vec4(0.0f),
                .borderWidth = 0.0f,
                .borderPosition = BorderPosition::Outside,
                .cornerRadius = 5.0f
            }),
            .onClick = [this]() {
                std::cout << "Developer button clicked!" << std::endl;
                screenManager->switchScreen(ScreenType::Developer);
            }
        }
    );
    buttonLayer->addItem(developerButton);
}

MainMenuScreen::~MainMenuScreen() {
}

bool MainMenuScreen::initialize() {
    doLayout();
    return true;
}



void MainMenuScreen::update(float deltaTime) {

}

void MainMenuScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Render all layers in order
    backgroundLayer->render(false);
    buttonLayer->render(false);
    titleLayer->render(false);
}

void MainMenuScreen::handleInput(float deltaTime) {
    buttonLayer->handleInput(deltaTime);
}

void MainMenuScreen::doLayout() {
    GLFWwindow* window = screenManager->getWindow(); // Still need window for size calculation
    int currentWidth, currentHeight;
    glfwGetWindowSize(window, &currentWidth, &currentHeight); // Use current size

    // TODO: Get the button height from a getter on the button
    int buttonHeight = 50;
    int buttonWidth = menuWidth - (menuPadding * 2);
    int menuHeight = (buttonHeight * 4) + (menuPadding * 5); // 4 buttons + padding
    glm::vec2 menuPosition = glm::vec2((currentWidth - menuWidth) / 2, (currentHeight - menuHeight) / 2);
    menuBackground->setPosition(menuPosition);
    menuBackground->setSize(glm::vec2(menuWidth, menuHeight));

    // Position buttons vertically stacked in the menu
    float currentY = menuPosition.y + menuPadding;
    float buttonX = menuPosition.x + menuPadding;

    newColonyButton->setPosition(glm::vec2(buttonX, currentY));
    newColonyButton->setSize(glm::vec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + buttonSpacing;
    
    loadColonyButton->setPosition(glm::vec2(buttonX, currentY));
    loadColonyButton->setSize(glm::vec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + buttonSpacing;
    
    settingsButton->setPosition(glm::vec2(buttonX, currentY));
    settingsButton->setSize(glm::vec2(buttonWidth, buttonHeight));
    currentY += buttonHeight + buttonSpacing;
    
    developerButton->setPosition(glm::vec2(buttonX, currentY));
    developerButton->setSize(glm::vec2(buttonWidth, buttonHeight));
}

void MainMenuScreen::onResize(int width, int height) {
    doLayout();
}

