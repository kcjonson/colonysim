#include "Game.h"
#include "ConfigManager.h"
#include <iostream>
#include <chrono>
#include <thread>

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

    // Initial render
    render();
    glfwSwapBuffers(window);
    
    // Fixed timestep variables
    const float fixedTimeStep = 1.0f / 60.0f;  // 60 FPS
    float accumulator = 0.0f;
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window) && isRunning) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // Cap delta time to prevent spiral of death
        if (deltaTime > 0.25f) {
            std::cout << "Delta time is too high: " << deltaTime << std::endl;
            deltaTime = 0.25f;
        }
        
        accumulator += deltaTime;
        
        // Process input
        processInput();
        
        // Fixed timestep updates
        while (accumulator >= fixedTimeStep) {
            // Debug FPS - reduced frequency
            static float fpsUpdateTime = 0.0f;
            static int frameCount = 0;
            fpsUpdateTime += deltaTime;
            frameCount++;
            
            if (fpsUpdateTime >= 5.0f) {  // Changed from 1.0f to 5.0f
                std::cout << "FPS: " << frameCount / fpsUpdateTime << std::endl;
                fpsUpdateTime = 0.0f;
                frameCount = 0;
            }

            // Update
            // Note: the fixedTimeStep is the time it took(ish)? to update the game state
            // this is important to know when calculating things like movement distance.
            update(fixedTimeStep);

            accumulator -= fixedTimeStep;
        }

        // Render
        render();

        // Swap buffers and poll events only if enough time has passed
        if (accumulator < fixedTimeStep) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            glfwSwapBuffers(window);
            vectorGraphics.clear(); // <-- SUPER FUCKING IMPORTANT
            auto frameEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> frameDuration = frameEnd - frameStart;
        }
        glfwPollEvents();
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
}

void Game::render() {

    // Set clear color to teal
    // this seems to be the global background color
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Get camera matrices once per frame
    const glm::mat4& viewMatrix = camera.getViewMatrix();
    const glm::mat4& projectionMatrix = camera.getProjectionMatrix();

    // Render world
    world.render(vectorGraphics);

    // Render all vector graphics in a single batch
    vectorGraphics.render(viewMatrix, projectionMatrix);
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
    }
}

void Game::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->inputManager.handleMouseButton(button, action);
    }
} 