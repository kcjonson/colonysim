// filepath: c:\Users\kevin\ColonySim\src\Screens\Game\Game.cpp
#include "Game.h"
#include "../ScreenManager.h"
#include "World.h"
#include "Entities.h"
#include "../../InputManager.h"
#include "Interface.h"
#include "../../GameState.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// Constructor that takes pointers to shared resources managed by ScreenManager
GameScreen::GameScreen(Camera* camera, GLFWwindow* window)
    : camera_(camera)
    , window_(window)
    , gameState_(nullptr)
    , isRunning(true)
    , timeSinceLastRenderLog(0.0f) {
    std::cout << "Creating GameScreen..." << std::endl;
}

GameScreen::~GameScreen() {
    // unique_ptr members will be automatically cleaned up
    std::cout << "GameScreen destroyed." << std::endl;
}

bool GameScreen::initialize() {
    std::cout << "Initializing GameScreen..." << std::endl;
    
    // Get necessary pointers from ScreenManager
    if (!screenManager) {
        std::cerr << "ERROR: ScreenManager is null in GameScreen::initialize" << std::endl;
        return false;
    }
    
    // Update pointers to shared resources
    camera_ = screenManager->getCamera();
    window_ = screenManager->getWindow();
    gameState_ = screenManager->getGameState();

    if (!camera_ || !window_ || !gameState_) {
        std::cerr << "ERROR: Failed to get required pointers (Camera, Window, GameState) from ScreenManager" << std::endl;
        return false;
    }

    // Initialize Interface
    try {
        interface_ = std::make_unique<Interface>(*gameState_, camera_, window_);
        if (!interface_ || !interface_->initialize()) {
            std::cerr << "ERROR: Interface initialization failed in GameScreen!" << std::endl;
            return false;
        }
        std::cout << "Interface initialized successfully in GameScreen." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Interface initialization in GameScreen: " << e.what() << std::endl;
        return false;
    }

    // Initialize Entities
    try {
        entities_ = std::make_unique<Entities>(camera_, window_);
        if (!entities_) {
             std::cerr << "ERROR: Failed to create Entities in GameScreen" << std::endl;
             return false;
        }
        std::cout << "Entities initialized successfully in GameScreen." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Entities initialization in GameScreen: " << e.what() << std::endl;
        return false;
    }

    std::cout << "GameScreen initialization complete." << std::endl;
    return true;
}

void GameScreen::update(float deltaTime) {
    // Calculate render stats
    timeSinceLastRenderLog += deltaTime;
    if (timeSinceLastRenderLog >= 1.0f) { // Log every 1 second
        timeSinceLastRenderLog = 0.0f;
        // Render stats logging would happen here
    }
    
    processInput();
    
    // Update game components using local members and shared resources from ScreenManager
    World* world = screenManager->getWorld();
    InputManager* inputManager = screenManager->getInputManager();

    if (inputManager) {
        inputManager->update(deltaTime);
    }

    if (world) {
        world->update(deltaTime);
    }

    if (entities_) {
        entities_->update(deltaTime);
    }

    if (interface_) {
        interface_->update(deltaTime);
    }
}

void GameScreen::render() {
    try {
        // Get framebuffer size for viewport (physical pixels)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window_, &fbWidth, &fbHeight);
        
        // Set viewport to full framebuffer size
        glViewport(0, 0, fbWidth, fbHeight);
        
        // Clear screen with black background
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Render game components in correct order using local members
        World* world = screenManager ? screenManager->getWorld() : nullptr;

        // First batch: Render world (background layer)
        if (world) {
            try {
                std::cout << "Rendering world..." << std::endl;
                world->render();
                std::cout << "World rendered successfully" << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception during world rendering: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ERROR: Unknown exception during world rendering" << std::endl;
            }
        } else {
            std::cerr << "WARNING: World is null in GameScreen::render" << std::endl;
        }

        // Second batch: Render entities (foreground layer)
        if (entities_) {
            try {
                entities_->render();
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception during entities rendering: " << e.what() << std::endl;
            }
        }

        // Third batch: Render interface elements (top layer)
        if (interface_) {
            try {
                interface_->render();
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception during interface rendering: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception in GameScreen::render: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "ERROR: Unknown exception in GameScreen::render" << std::endl;
    }
}

void GameScreen::processInput() {
    // This would hold keyboard/mouse processing that was in the original Game class
    // For now, this handles basic window-closing input
    if (window_ && glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        isRunning = false;
    }
}

void GameScreen::handleInput(float deltaTime) {
    // Use the stored window_ pointer
    if (!window_) return;

    // Check for ESC key to go back to world gen menu
    if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        screenManager->switchScreen(ScreenType::WorldGen);
    }

    // All other input is handled by the InputManager
    // which is updated in the update method
}

void GameScreen::onResize(int width, int height) {
    // Handle resize events
    // Note: width and height are in logical pixels (window coordinates)
    if (camera_) {
        camera_->setOrthographicProjection(
            -width / 2.0f, width / 2.0f,
            -height / 2.0f, height / 2.0f,
            -1000.0f, 1000.0f
        );
    }
    
    // The viewport is set in render() using framebuffer size
}

// Static callback for GLFW that was in the original Game class
void GameScreen::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    // Assuming we'd handle this through the onResize method
}