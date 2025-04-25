#include "Splash.h"
#include "../ScreenManager.h"
#include "../../VectorGraphics.h"
#include "../../ConfigManager.h"
#include <glad/glad.h>
#include <iostream>
#include "../../Camera.h" // Include Camera header

// Update constructor to accept Camera* and GLFWwindow*
SplashScreen::SplashScreen(Camera* camera, GLFWwindow* window)
    : displayDuration(5.0f)
    , title("ColonySim")
    // REMOVED: , openglInitialized(false)
    , clicked(false) {
    
    // REMOVED: Pass nullptr for camera/window initially
    // Camera* camera = nullptr; // No longer needed
    // GLFWwindow* window = nullptr; // No longer needed

    // Create layer for splash screen content with high z-index and pass pointers
    splashLayer = std::make_shared<Rendering::Layer>(100.0f, Rendering::ProjectionType::ScreenSpace, camera, window);
}

SplashScreen::~SplashScreen() {
}

bool SplashScreen::initialize() {
    // Record the start time
    startTime = std::chrono::steady_clock::now();
    
    // Get window dimensions
    GLFWwindow* window = screenManager->getWindow();
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // REMOVED: Set window reference for layer
    // splashLayer->setWindow(window);
    
    // Create title text
    titleText = std::make_shared<Rendering::Shapes::Text>(
        title,
        glm::vec2(width / 2.0f, height / 2.0f),
        Rendering::Styles::Text({
            .color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            .fontSize = 48.0f,
            .horizontalAlign = Rendering::TextAlign::Center,
            .verticalAlign = Rendering::TextAlign::Middle
        }),
        110.0f  // Z-index
    );
    splashLayer->addItem(titleText);

    // Manually initialize the main menu screen
    // move to end of any heavy loading we need to do.
    screenManager->initializeScreen(ScreenType::MainMenu);
    
    return true;
}

void SplashScreen::update(float deltaTime) {
    // REMOVED: Check if OpenGL needs to be initialized
    // if (!openglInitialized) {
    //     // Initialize OpenGL components
    //     if (screenManager->initializeOpenGL()) {
    //         openglInitialized = true;
    //     }
    // }
    
    // Calculate elapsed time
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedSecs = std::chrono::duration<float>(currentTime - startTime).count();
    
    // If time has elapsed or user clicked, go to main menu
    // REMOVED: && openglInitialized check
    if (elapsedSecs >= displayDuration || clicked) {
        screenManager->switchScreen(ScreenType::MainMenu);
    }
}

void SplashScreen::render() {
    // Set clear color to black
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // REMOVED: Only render if OpenGL is initialized
    // if (openglInitialized) {
    splashLayer->render(false); // No camera transform
    // }
}

void SplashScreen::handleInput() {
    // Check for any mouse click to dismiss the splash screen
    GLFWwindow* window = screenManager->getWindow();
    if (window) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
            glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            clicked = true;
        }
    }
}

void SplashScreen::onResize(int width, int height) {
    // REMOVED: Set window reference for layer again after resize
    // GLFWwindow* window = screenManager->getWindow();
    // splashLayer->setWindow(window);
    
    // Update the position of the title text
    if (titleText) {
        titleText->setPosition(glm::vec2(width / 2.0f, height / 2.0f));
        // If Layer::addItem doesn't update window/camera on children, we might need:
        // titleText->setWindow(screenManager->getWindow()); // Or similar, depending on Shape interface
    }
}