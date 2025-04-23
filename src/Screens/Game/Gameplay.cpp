#include "Gameplay.h"
#include "../ScreenManager.h"
#include "World.h"
#include "Entities.h"
#include "../../InputManager.h"
#include "Interface.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Update constructor definition to accept Camera* and GLFWwindow*
GameplayScreen::GameplayScreen(Camera* camera, GLFWwindow* window)
    : camera_(camera)
    , window_(window) {
    // Constructor body (can be empty if initialization list is sufficient)
}

GameplayScreen::~GameplayScreen() {
}

bool GameplayScreen::initialize() {
    // Gameplay screen initialization
    // No additional initialization needed as the world is already created
    return true;
}

void GameplayScreen::update(float deltaTime) {
    // Update game components
    InputManager* inputManager = screenManager->getInputManager();
    World* world = screenManager->getWorld();
    Entities* entities = screenManager->getEntities();
    Interface* interface = screenManager->getInterface();
    
    if (inputManager) {
        inputManager->update(deltaTime);
    }
    
    if (world) {
        world->update(deltaTime);
    }
    
    if (entities) {
        entities->update(deltaTime);
    }
    
    if (interface) {
        interface->update(deltaTime);
    }
}

void GameplayScreen::render() {
    // Clear screen with black background
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Setup OpenGL state for rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Render game components in correct order
    World* world = screenManager->getWorld();
    Entities* entities = screenManager->getEntities();
    Interface* interface = screenManager->getInterface();
    
    // First batch: Render world (background layer)
    if (world) {
        world->render();
    }
    
    // Second batch: Render entities (foreground layer)
    if (entities) {
        entities->render();
    }
    
    // Third batch: Render interface elements (top layer)
    if (interface) {
        interface->render();
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