#include "Gameplay.h"
#include "../ScreenManager.h"
#include "World.h"
#include "Entities.h" // Include Entities header
#include "../../InputManager.h"
#include "Interface.h" // Include Interface header
#include "../../GameState.h" // Include GameState header
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Update constructor definition to accept Camera*, GLFWwindow*, and GameState*
GameplayScreen::GameplayScreen(Camera* camera, GLFWwindow* window)
    : camera_(camera)
    , window_(window)
    , gameState_(nullptr) { // Initialize gameState_ to nullptr
    // Constructor body
}

GameplayScreen::~GameplayScreen() {
    // unique_ptr members will be automatically cleaned up
}

bool GameplayScreen::initialize() {
    std::cout << "Initializing GameplayScreen..." << std::endl;
    // Get necessary pointers from ScreenManager
    if (!screenManager) {
        std::cerr << "ERROR: ScreenManager is null in GameplayScreen::initialize" << std::endl;
        return false;
    }
    camera_ = screenManager->getCamera();
    window_ = screenManager->getWindow();
    gameState_ = screenManager->getGameState(); // Get GameState

    if (!camera_ || !window_ || !gameState_) {
        std::cerr << "ERROR: Failed to get required pointers (Camera, Window, GameState) from ScreenManager" << std::endl;
        return false;
    }

    // Initialize Interface
    try {
        interface_ = std::make_unique<Interface>(*gameState_, camera_, window_);
        if (!interface_ || !interface_->initialize()) {
            std::cerr << "ERROR: Interface initialization failed in GameplayScreen!" << std::endl;
            return false;
        }
        std::cout << "Interface initialized successfully in GameplayScreen." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Interface initialization in GameplayScreen: " << e.what() << std::endl;
        return false;
    }

    // Initialize Entities
    try {
        entities_ = std::make_unique<Entities>(camera_, window_);
        if (!entities_) {
             std::cerr << "ERROR: Failed to create Entities in GameplayScreen" << std::endl;
             return false;
        }
        // Entities might not have an initialize() method, assuming creation is enough
        std::cout << "Entities initialized successfully in GameplayScreen." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Entities initialization in GameplayScreen: " << e.what() << std::endl;
        return false;
    }

    std::cout << "GameplayScreen initialization complete." << std::endl;
    return true;
}

void GameplayScreen::update(float deltaTime) {
    // Update game components using local members
    InputManager* inputManager = screenManager->getInputManager(); // Still get InputManager from ScreenManager
    World* world = screenManager->getWorld(); // Still get World from ScreenManager

    if (inputManager) {
        inputManager->update(deltaTime);
    }

    if (world) {
        world->update(deltaTime);
    }

    if (entities_) { // Use local member
        entities_->update(deltaTime);
    }

    if (interface_) { // Use local member
        interface_->update(deltaTime);
    }
}

void GameplayScreen::render() {
    // Clear screen with black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Render game components in correct order using local members
    World* world = screenManager->getWorld(); // Still get World from ScreenManager

    // First batch: Render world (background layer)
    if (world) {
        world->render();
    }

    // Second batch: Render entities (foreground layer)
    if (entities_) { // Use local member
        entities_->render();
    }

    // Third batch: Render interface elements (top layer)
    if (interface_) { // Use local member
        interface_->render();
    }
}

void GameplayScreen::handleInput() {
    // Use the stored window_ pointer
    if (!window_) return; // Add null check

    // Check for ESC key to go back to world gen menu
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::WorldGen);
    }

    // All other input is handled by the InputManager
    // which is updated in the update method
}

void GameplayScreen::onResize(int width, int height) {
    // Resize handling is done globally by ScreenManager
    // No need to use camera_ or window_ here directly unless specific logic is needed
}