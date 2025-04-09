#include "Game.h"
#include "ConfigManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
// Include shape classes
#include "Rendering/Shapes/Rectangle.h"
#include "Rendering/Shapes/Circle.h"
#include "Rendering/Shapes/Line.h"
#include "Rendering/Shapes/Polygon.h"
#include "Rendering/Shapes/Text.h"

Game::Game() 
    : window(nullptr)
    , camera()
    , world()
    , inputManager(nullptr, camera, world.getEntityManager())
    , vectorGraphics()
    , isRunning(true) {
    
    std::cout << "Initializing game..." << std::endl;
    
    // Load configuration
    if (!ConfigManager::getInstance().loadConfig("config/game_config.json")) {
        std::cerr << "Failed to load configuration, using defaults" << std::endl;
    }
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }
    std::cout << "GLFW initialized successfully" << std::endl;

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window using config settings
    window = glfwCreateWindow(
        ConfigManager::getInstance().getWindowWidth(),
        ConfigManager::getInstance().getWindowHeight(),
        ConfigManager::getInstance().getWindowTitle().c_str(),
        nullptr,
        nullptr
    );
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return;
    }

    // Initialize VectorGraphics after GLAD is initialized
    if (!vectorGraphics.initialize()) {
        std::cerr << "ERROR: VectorGraphics initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }

    // Initialize Interface
    if (!interface.initialize()) {
        std::cerr << "ERROR: Interface initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }
    
    // Initialize Interface graphics
    if (!interface.initializeGraphics(window)) {
        std::cerr << "ERROR: Interface graphics initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }

    // Set up viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Set up camera projection using config settings
    float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    float viewHeight = ConfigManager::getInstance().getViewHeight();
    float viewWidth = viewHeight * aspectRatio;
    camera.setOrthographicProjection(
        -viewWidth/2.0f, viewWidth/2.0f,
        -viewHeight/2.0f, viewHeight/2.0f,
        ConfigManager::getInstance().getNearPlane(),
        ConfigManager::getInstance().getFarPlane()
    );

    // Set up callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Generate terrain
    world.generateTerrain();
    
    // Initialize World
    if (!world.initialize()) {
        std::cerr << "ERROR: World initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }

    // Create some test entities
    std::cout << "Creating test entities..." << std::endl;
    
    // Create a worker entity
    size_t workerId = world.createEntity(
        glm::vec2(0.0f, 0.0f),
        glm::vec2(20.0f, 20.0f),
        glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) // Green
    );
    Entity* worker = world.getEntity(workerId);
    if (worker) {
        worker->setSpeed(100.0f);
        worker->setState(EntityState::IDLE);
        worker->setType(EntityType::WORKER);
    }

    // Create a resource entity
    size_t resourceId = world.createEntity(
        glm::vec2(100.0f, 100.0f),
        glm::vec2(30.0f, 30.0f),
        glm::vec4(1.0f, 0.5f, 0.0f, 1.0f) // Orange
    );
    Entity* resource = world.getEntity(resourceId);
    if (resource) {
        resource->setState(EntityState::IDLE);
        resource->setType(EntityType::RESOURCE);
    }

    // Create a building entity
    size_t buildingId = world.createEntity(
        glm::vec2(-100.0f, -100.0f),
        glm::vec2(50.0f, 50.0f),
        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) // Gray
    );
    Entity* building = world.getEntity(buildingId);
    if (building) {
        building->setState(EntityState::IDLE);
        building->setType(EntityType::BUILDING);
    }

    // Set the window in InputManager after everything is initialized
    inputManager.setWindow(window);

    std::cout << "Game initialization complete" << std::endl;
}

Game::~Game() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void Game::run() {
    if (!window) {
        std::cerr << "Cannot run game: window not initialized" << std::endl;
        return;
    }

    std::cout << "Starting game loop..." << std::endl;

    
    // Fixed timestep variables
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window) && isRunning) {
        // Start timing
        double frameStartTime = glfwGetTime();

        // Calculate delta time
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // Cap delta time
        // Large delta times can cause calculations in game state to get out of sync.
        // This will deliberately limit how much time can pass between updates.
        // I think this will result in behavior like entities moving in small steps
        // rather than large jumps as FPS drops.
        if (deltaTime > 0.25f) {
            deltaTime = 0.25f;  // Prevent large jumps
        }

        // Process input
        processInput();

        // Update game state
        update(deltaTime);

        // Render the frame
        render();

        // Swap buffers
        glfwSwapBuffers(window);

        // Poll events
        glfwPollEvents();

        // End timing
        double frameEndTime = glfwGetTime();
        double frameDuration = frameEndTime - frameStartTime;

        // Calculate FPS
        if (frameDuration > 0.0) {
            float fps = 1.0f / static_cast<float>(frameDuration);
            // std::cout << "FPS: " << fps << std::endl;  // Output FPS
            interface.setFPS(fps);
        }
    }
    
    std::cout << "Game loop ended" << std::endl;
}

void Game::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        isRunning = false;
    }
}

void Game::update(float deltaTime) {
    inputManager.update(deltaTime);
    world.update(deltaTime);
    interface.setCursorWorldPosition(inputManager.getCursorWorldPos());
    interface.update(deltaTime);
}

void Game::render() {
    // Set clear color to teal
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Get camera matrices once per frame
    const glm::mat4& viewMatrix = camera.getViewMatrix();
    const glm::mat4& projectionMatrix = camera.getProjectionMatrix();

    // Create screen-space projection matrix manually for UI
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glm::mat4 screenProjection = glm::mat4(1.0f);
    screenProjection[0][0] = 2.0f / width;  // Scale x
    screenProjection[1][1] = -2.0f / height; // Scale y (negative to flip y-axis)
    screenProjection[3][0] = -1.0f;          // Translate x
    screenProjection[3][1] = 1.0f;           // Translate y

    // Start a new batch for world rendering
    vectorGraphics.beginBatch();
    
    // Render world (will add to the current batch)
    world.render(vectorGraphics, viewMatrix, projectionMatrix);
    
    // End batch and render world
    vectorGraphics.endBatch();
    vectorGraphics.render(viewMatrix, projectionMatrix);
    
    // Start a new batch for UI rendering
    vectorGraphics.beginBatch();
    
    // Render UI (will add to the current batch)
    interface.render(vectorGraphics, screenProjection);
    
    // End batch and render UI
    vectorGraphics.endBatch();
    vectorGraphics.render(glm::mat4(1.0f), screenProjection); // Identity view matrix for screen space
}

void Game::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    
    // Update camera projection
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
        float viewHeight = ConfigManager::getInstance().getViewHeight();
        float viewWidth = viewHeight * aspectRatio;
        game->camera.setOrthographicProjection(
            -viewWidth/2.0f, viewWidth/2.0f,
            -viewHeight/2.0f, viewHeight/2.0f,
            ConfigManager::getInstance().getNearPlane(),
            ConfigManager::getInstance().getFarPlane()
        );
    }
}

void Game::mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->inputManager.handleScroll(xoffset, yoffset);
    }
}

void Game::mouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->inputManager.handleMouseMove(xpos, ypos);
        
        // Convert screen coordinates to world coordinates
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        
        // Convert to normalized device coordinates
        float ndcX = (2.0f * static_cast<float>(xpos)) / static_cast<float>(width) - 1.0f;
        float ndcY = 1.0f - (2.0f * static_cast<float>(ypos)) / static_cast<float>(height);
        
        // Convert to world coordinates using camera matrices
        glm::vec4 worldPos = glm::inverse(game->camera.getProjectionMatrix() * game->camera.getViewMatrix()) * 
                            glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        
        game->interface.setCursorWorldPosition(glm::vec2(worldPos.x, worldPos.y));
    }
}

void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->inputManager.handleMouseButton(button, action);
    }
}