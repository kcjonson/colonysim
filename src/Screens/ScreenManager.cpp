#include "ScreenManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "../Camera.h"
#include "../GameState.h"
#include "Game/World.h"
#include "Game/Entities.h"
#include "../InputManager.h"
#include "Game/Interface.h"
#include "../ConfigManager.h"
#include "../VectorGraphics.h"
#include "../Renderer.h"
#include "Developer/Examples.h"

// Include all screen types
#include "Splash/Splash.h"
#include "MainMenu/MainMenu.h"
#include "WorldGen/WorldGen.h"
#include "Game/Gameplay.h"
#include "Settings/Settings.h"
#include "Developer/Developer.h"

ScreenManager::ScreenManager()
    : window(nullptr)
    , currentScreen(nullptr)
    , isRunning(true)
    , openglInitialized(false) {
}

ScreenManager::~ScreenManager() {
    cleanup();
}

bool ScreenManager::initialize() {
    std::cout << "Initializing ScreenManager..." << std::endl;

    // Create GLFW window with black background
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    std::cout << "GLFW initialized successfully" << std::endl;

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

    // Make sure the window background is black
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);

    // Load configuration
    if (!ConfigManager::getInstance().loadConfig("config/game_config.json")) {
        std::cerr << "Failed to load configuration, using defaults" << std::endl;
    }

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
        return false;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);
    
    // Set the window's user pointer to this instance
    glfwSetWindowUserPointer(window, this);
    
    // Set up callbacks
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    // Don't make OpenGL calls here as GLAD hasn't been initialized yet
    // Just swap buffers to show the black window created by GLFW
    glfwSwapBuffers(window);
    
    // Create shared game state first
    gameState = std::make_unique<GameState>();
    
    // Create splash screen immediately (it doesn't need OpenGL fully initialized)
    screens[ScreenType::Splash] = std::make_unique<SplashScreen>();
    screens[ScreenType::Splash]->setScreenManager(this);
    
    if (!screens[ScreenType::Splash]->initialize()) {
        std::cerr << "Failed to initialize splash screen" << std::endl;
        return false;
    }
    
    // Set current screen to splash screen
    currentScreen = screens[ScreenType::Splash].get();
    
    return true;
}

bool ScreenManager::initializeOpenGL() {
    if (openglInitialized) return true;
    
    std::cout << "Initializing OpenGL..." << std::endl;
    
    // Check if window is valid
    if (!window) {
        std::cerr << "ERROR: Window is null in initializeOpenGL" << std::endl;
        return false;
    }
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Set up viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    
    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Enable anti-aliasing
    glEnable(GL_MULTISAMPLE);
    
    // Initialize renderer first
    if (!Renderer::getInstance().initialize()) {
        std::cerr << "ERROR: Renderer initialization failed!" << std::endl;
        return false;
    }

    // Set VectorGraphics to use the renderer
    VectorGraphics::getInstance().setRenderer(&Renderer::getInstance());
    
    // Initialize VectorGraphics after GLAD is initialized
    if (!VectorGraphics::getInstance().initialize()) {
        std::cerr << "ERROR: VectorGraphics initialization failed!" << std::endl;
        return false;
    }
    
    // Check if gameState exists
    if (!gameState) {
        std::cerr << "ERROR: GameState is null in initializeOpenGL" << std::endl;
        return false;
    }
    
    // Initialize camera
    camera = std::make_unique<Camera>();
    if (!camera) {
        std::cerr << "ERROR: Failed to create Camera" << std::endl;
        return false;
    }
    
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    camera->setOrthographicProjection(
        -halfWidth, halfWidth,
        -halfHeight, halfHeight,
        -1000.0f, 1000.0f
    );
    
    // Initialize interface
    interface = std::make_unique<Interface>(*gameState);
    if (!interface) {
        std::cerr << "ERROR: Failed to create Interface" << std::endl;
        return false;
    }
    
    if (!interface->initialize() || !interface->initializeGraphics(window)) {
        std::cerr << "ERROR: Interface initialization failed!" << std::endl;
        return false;
    }
    
    // Initialize world (but don't generate terrain yet)
    world = std::make_unique<World>(*gameState);
    if (!world) {
        std::cerr << "ERROR: Failed to create World" << std::endl;
        return false;
    }
    
    world->setCamera(camera.get());
    world->setWindow(window);
    if (!world->initialize()) {
        std::cerr << "ERROR: World initialization failed!" << std::endl;
        return false;
    }
    
    // Initialize entities
    entities = std::make_unique<Entities>();
    if (!entities) {
        std::cerr << "ERROR: Failed to create Entities" << std::endl;
        return false;
    }
    
    entities->setCamera(camera.get());
    entities->setWindow(window);
    
    // Initialize input manager
    try {
        inputManager = std::make_unique<InputManager>(window, *camera, *entities, *gameState);
        if (!inputManager) {
            std::cerr << "ERROR: Failed to create InputManager" << std::endl;
            return false;
        }
        inputManager->setWindow(window);
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create InputManager: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "ERROR: Unknown exception when creating InputManager" << std::endl;
        return false;
    }
    
    // Initialize examples for developer screen
    examples = std::make_unique<Examples>();
    if (!examples) {
        std::cerr << "ERROR: Failed to create Examples" << std::endl;
        return false;
    }
    
    // Call initialize() but don't use it in a condition since it returns void
    examples->initialize();
    
    // Create the remaining screens
    try {
        screens[ScreenType::MainMenu] = std::make_unique<MainMenuScreen>();
        screens[ScreenType::WorldGen] = std::make_unique<WorldGenScreen>();
        screens[ScreenType::Gameplay] = std::make_unique<GameplayScreen>();
        screens[ScreenType::Settings] = std::make_unique<SettingsScreen>();
        screens[ScreenType::Developer] = std::make_unique<DeveloperScreen>();
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create screens: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "ERROR: Unknown exception when creating screens" << std::endl;
        return false;
    }
    
    // Set the screen manager reference for each screen
    for (auto& [type, screen] : screens) {
        if (type != ScreenType::Splash) { // Splash already initialized
            if (!screen) {
                std::cerr << "ERROR: Screen is null for type: " << static_cast<int>(type) << std::endl;
                continue;
            }
            
            screen->setScreenManager(this);
            if (!screen->initialize()) {
                std::cerr << "Failed to initialize screen: " << static_cast<int>(type) << std::endl;
                return false;
            }
        }
    }
    
    openglInitialized = true;
    std::cout << "OpenGL initialization complete" << std::endl;
    return true;
}

void ScreenManager::run() {
    if (!window) {
        std::cerr << "Cannot run: window not initialized" << std::endl;
        return;
    }

    std::cout << "Starting game loop..." << std::endl;
    
    // Fixed timestep variables
    double lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window) && isRunning) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;
        
        // Cap delta time to prevent large jumps
        if (deltaTime > 0.25f) {
            deltaTime = 0.25f;
        }
        
        // Handle input for current screen
        if (currentScreen) {
            try {
                currentScreen->handleInput();
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception in handleInput: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ERROR: Unknown exception in handleInput" << std::endl;
            }
        }
        
        // Update current screen
        if (currentScreen) {
            try {
                currentScreen->update(deltaTime);
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception in update: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ERROR: Unknown exception in update" << std::endl;
            }
        }
        
        // Render current screen
        if (currentScreen) {
            try {
                currentScreen->render();
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception in render: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ERROR: Unknown exception in render" << std::endl;
            }
        }
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
        
        // Calculate FPS
        if (deltaTime > 0.0 && gameState) {
            try {
                float fps = 1.0f / deltaTime;
                gameState->set("system.fps", std::to_string(static_cast<int>(fps)));
            } catch (const std::exception& e) {
                std::cerr << "ERROR: Exception when calculating FPS: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "ERROR: Unknown exception when calculating FPS" << std::endl;
            }
        }
    }
    
    std::cout << "Game loop ended" << std::endl;
}

void ScreenManager::switchScreen(ScreenType screenType) {
    // Make sure OpenGL is initialized before switching to any screen other than splash
    if (screenType != ScreenType::Splash && !openglInitialized) {
        if (!initializeOpenGL()) {
            std::cerr << "Failed to initialize OpenGL, cannot switch screens" << std::endl;
            return;
        }
    }
    
    auto it = screens.find(screenType);
    if (it != screens.end()) {
        currentScreen = it->second.get();
        std::cout << "Switched to screen: " << static_cast<int>(screenType) << std::endl;
    } else {
        std::cerr << "Screen not found: " << static_cast<int>(screenType) << std::endl;
    }
}

void ScreenManager::cleanup() {
    // Clear screens first
    screens.clear();
    
    // Clear other resources
    examples.reset();
    inputManager.reset();
    entities.reset();
    world.reset();
    interface.reset();
    camera.reset();
    gameState.reset();
    
    // Destroy window and terminate GLFW
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void ScreenManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Update viewport
    glViewport(0, 0, width, height);
    
    ScreenManager* manager = static_cast<ScreenManager*>(glfwGetWindowUserPointer(window));
    if (manager && manager->currentScreen) {
        manager->currentScreen->onResize(width, height);
        
        // Also update camera projection if camera exists
        if (manager->camera) {
            float halfWidth = width / 2.0f;
            float halfHeight = height / 2.0f;
            
            manager->camera->setOrthographicProjection(
                -halfWidth, halfWidth,
                -halfHeight, halfHeight,
                -1000.0f, 1000.0f
            );
            
            std::cout << "Window resized to: " << width << "x" << height << std::endl;
        }
    }
}

void ScreenManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ScreenManager* manager = static_cast<ScreenManager*>(glfwGetWindowUserPointer(window));
    if (manager && manager->inputManager) {
        // Forward to input manager
        // This will be handled by the screen via handleInput()
    }
}

void ScreenManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ScreenManager* manager = static_cast<ScreenManager*>(glfwGetWindowUserPointer(window));
    if (manager && manager->inputManager) {
        // Forward to input manager
        // This will be handled by the screen via handleInput()
    }
}