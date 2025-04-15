#include "Game.h"
#include "../../ConfigManager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <glm/gtc/matrix_transform.hpp>
// Include shape classes
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Circle.h"
#include "../../Rendering/Shapes/Line.h"
#include "../../Rendering/Shapes/Polygon.h"
#include "../../Rendering/Shapes/Text.h"

Game::Game() 
    : window(nullptr)
    , camera()
    , gameState()
    , world(gameState)
    , entities()
    , inputManager(window, camera, entities, gameState)
    , interface(gameState)
    , isRunning(true)
    , timeSinceLastRenderLog(0.0f) {
    
    std::cout << "Initializing game..." << std::endl;
    
    // Set VectorGraphics to use the renderer
    VectorGraphics::getInstance().setRenderer(&Renderer::getInstance());
    
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
    
    // Enable anti-aliasing
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
    
    // Explicitly request an alpha channel
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

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
    
    // Initialize renderer first
    if (!Renderer::getInstance().initialize()) {
        std::cerr << "ERROR: Renderer initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }

    // Initialize VectorGraphics after GLAD is initialized
    if (!VectorGraphics::getInstance().initialize()) {
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
    
    // Initialize Interface graphics with window
    if (!interface.initializeGraphics(window)) {
        std::cerr << "ERROR: Interface graphics initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }

    // Set up viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Set up camera projection using direct window dimensions for 1:1 pixel-to-world mapping
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    
    // Debug output
    // std::cout << "Setting up camera projection:" << std::endl;
    // std::cout << "  Window size: " << width << "x" << height << std::endl;
    // std::cout << "  Projection: left=" << -halfWidth << ", right=" << halfWidth 
    //           << ", bottom=" << -halfHeight << ", top=" << halfHeight << std::endl;
    
    // Set camera projection to exactly match window dimensions in world units
    camera.setOrthographicProjection(
        -halfWidth, halfWidth,
        -halfHeight, halfHeight,
        -1000.0f, 1000.0f
    );

    // Set up callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable anti-aliasing
    glEnable(GL_MULTISAMPLE);


    
    // Initialize World (should be called after camera is set up)
    if (!world.initialize()) {
        std::cerr << "ERROR: World initialization failed!" << std::endl;
        glfwTerminate();
        return;
    }
    
    // Set up references for world and interface
    world.setCamera(&camera);
    world.setWindow(window);
    
    // Set up references for entities
    entities.setCamera(&camera);
    entities.setWindow(window);
    

    // Initialize Examples
    examples.initialize();

    // // Create some test entities
    // std::cout << "Creating test entities..." << std::endl;
    
    // // Create a worker entity
    // size_t workerId = entities.createEntity(
    //     glm::vec2(0.0f, 0.0f),
    //     glm::vec2(20.0f, 20.0f),
    //     glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) // Green
    // );
    // Entity* worker = entities.getEntity(workerId);
    // if (worker) {
    //     worker->setSpeed(100.0f);
    //     worker->setState(EntityState::IDLE);
    //     worker->setType(EntityType::WORKER);
    // }

    // // Create a resource entity
    // size_t resourceId = entities.createEntity(
    //     glm::vec2(100.0f, 100.0f),
    //     glm::vec2(30.0f, 30.0f),
    //     glm::vec4(1.0f, 0.5f, 0.0f, 1.0f) // Orange
    // );
    // Entity* resource = entities.getEntity(resourceId);
    // if (resource) {
    //     resource->setState(EntityState::IDLE);
    //     resource->setType(EntityType::RESOURCE);
    // }

    // // Create a building entity
    // size_t buildingId = entities.createEntity(
    //     glm::vec2(-100.0f, -100.0f),
    //     glm::vec2(50.0f, 50.0f),
    //     glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) // Gray
    // );
    // Entity* building = entities.getEntity(buildingId);
    // if (building) {
    //     building->setState(EntityState::IDLE);
    //     building->setType(EntityType::BUILDING);
    // }

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
    
    // Initialize the screen manager
    screenManager = std::make_unique<ScreenManager>();
    screenManager->initialize();
    screenManager->run();
    
    std::cout << "Game loop ended" << std::endl;
}

void Game::processInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        isRunning = false;
    }
}

void Game::update(float deltaTime) {
    // Update input manager
    inputManager.update(deltaTime);
    
    // Update world
    world.update(deltaTime);
    
    // Update entities
    entities.update(deltaTime);
    
    // Update interface
    interface.update(deltaTime);
    
    // Log rendering statistics every 0.5 seconds
    timeSinceLastRenderLog += deltaTime;
    if (timeSinceLastRenderLog >= 0.5f) {
        // Log rendering statistics
        size_t totalVertices = VectorGraphics::getInstance().getTotalVertices();
        size_t totalIndices = VectorGraphics::getInstance().getTotalIndices();
        gameState.set("rend.vertices", std::to_string(totalVertices));
        gameState.set("rend.indices", std::to_string(totalIndices));

        timeSinceLastRenderLog = 0.0f; // Reset the timer
    }
}

void Game::render() {
    // The ScreenManager now handles all rendering
    // This method is kept for backward compatibility but shouldn't be called
    std::cerr << "Warning: Game::render() called directly instead of using ScreenManager" << std::endl;
}


// This stuff is making it possible to resize the window ... somehow.
void Game::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Update viewport to match window dimensions exactly
    glViewport(0, 0, width, height);
    
    // Update camera projection
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        // Use direct window dimensions for 1:1 pixel-to-world mapping
        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;
        
        // Debug output
        // std::cout << "Window resized to: " << width << "x" << height << std::endl;
        // std::cout << "  New projection: left=" << -halfWidth << ", right=" << halfWidth 
        //           << ", bottom=" << -halfHeight << ", top=" << halfHeight << std::endl;
        
        // Set camera projection to exactly match window dimensions in world units
        std::cout << "Window resized to: " << width << "x" << height << std::endl;
        std::cout << "  New projection: left=" << -halfWidth << ", right=" << halfWidth 
                  << ", bottom=" << -halfHeight << ", top=" << halfHeight << std::endl;
        game->camera.setOrthographicProjection(
            -halfWidth, halfWidth,
            -halfHeight, halfHeight,
            -1000.0f, 1000.0f
        );
    }
}