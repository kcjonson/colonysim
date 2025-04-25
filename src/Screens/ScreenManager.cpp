#include "ScreenManager.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "../Camera.h"
#include "../GameState.h"
#include "Game/World.h"
// Removed include: #include "Game/Entities.h"
#include "../InputManager.h"
// Removed include: #include "Game/Interface.h"
#include "../ConfigManager.h"
#include "../VectorGraphics.h"
#include "../Renderer.h"
#include "Developer/Examples.h"

// Include all screen types
#include "Splash/Splash.h"
#include "MainMenu/MainMenu.h"
#include "WorldGen/WorldGen.h"
#include "Game/Game.h" // Updated to use Game.h instead of Gameplay.h
#include "Settings/Settings.h"
#include "Developer/Developer.h"

// Constructor that accepts a required GameState pointer
ScreenManager::ScreenManager(GameState* initializedGameState)
    : window(nullptr)
    , currentScreen(nullptr)
    , isRunning(true) {
    // GameState is now required
    if (!initializedGameState) {
        throw std::runtime_error("GameState must be provided to ScreenManager constructor");
    }
    gameState = std::unique_ptr<GameState>(initializedGameState);
    std::cout << "Using provided GameState" << std::endl;
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

    // *** Initialize OpenGL and core components immediately ***
    if (!initializeOpenGL()) {
        std::cerr << "Failed to initialize OpenGL components" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return false;
    }

    // *** Create screen factories (do NOT create screens yet) ***
    screenFactories[ScreenType::Splash] = [this]() { return std::make_unique<SplashScreen>(camera.get(), window); };
    screenFactories[ScreenType::MainMenu] = [this]() { return std::make_unique<MainMenuScreen>(camera.get(), window); };
    screenFactories[ScreenType::WorldGen] = [this]() { return std::make_unique<WorldGenScreen>(camera.get(), window); };
    screenFactories[ScreenType::Gameplay] = [this]() { return std::make_unique<GameScreen>(camera.get(), window); };
    screenFactories[ScreenType::Settings] = [this]() { return std::make_unique<SettingsScreen>(camera.get(), window); };
    screenFactories[ScreenType::Developer] = [this]() { return std::make_unique<DeveloperScreen>(camera.get(), window); };

    // Only create and initialize the splash screen at startup
    initializeScreen(ScreenType::Splash);
    currentScreen = activeScreens[ScreenType::Splash].get();
    if (!currentScreen) {
        std::cerr << "ERROR: Splash screen is null after creation" << std::endl;
        return false;
    }

    std::cout << "ScreenManager initialization complete." << std::endl;
    return true;
}

bool ScreenManager::initializeOpenGL() {
    std::cout << "Initializing OpenGL and core components..." << std::endl;

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
    
    // Initialize world (pass gameState, camera, and window)
    try {
        world = std::make_unique<World>(*gameState, "default_seed", camera.get(), window);
        if (!world) {
            std::cerr << "ERROR: Failed to create World" << std::endl;
            return false;
        }
        
        if (!world->initialize()) {
            std::cerr << "ERROR: World initialization failed!" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during World initialization: " << e.what() << std::endl;
        return false;
    }
    
    // Initialize input manager
    try {
        // Pass camera reference, window pointer, and gameState (Entities removed)
        inputManager = std::make_unique<InputManager>(window, *camera, *gameState); // Removed entities
        if (!inputManager) {
            std::cerr << "ERROR: Failed to create InputManager" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during InputManager initialization: " << e.what() << std::endl;
        return false;
    }
    
    // Initialize examples for developer screen (pass camera and window)
    try {
        examples = std::make_unique<Examples>(camera.get(), window);
        if (!examples) {
            std::cerr << "ERROR: Failed to create Examples" << std::endl;
            return false;
        }
        
        examples->initialize();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Examples initialization: " << e.what() << std::endl;
        return false;
    }
    
    std::cout << "OpenGL and core component initialization complete" << std::endl;
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
                // Reset frame counters before rendering starts
                VectorGraphics::getInstance().beginFrame(); 
                
                currentScreen->render();

                // Update GameState with rendering stats AFTER rendering is done
                if (gameState) {
                    try {
                        size_t vertices = VectorGraphics::getInstance().getFrameVertices();
                        size_t indices = VectorGraphics::getInstance().getFrameIndices();
                        gameState->set("rend.vertices", std::to_string(vertices));
                        gameState->set("rend.indices", std::to_string(indices));
                    } catch (const std::exception& e) {
                        std::cerr << "ERROR: Exception when updating render stats: " << e.what() << std::endl;
                    } catch (...) {
                        std::cerr << "ERROR: Unknown exception when updating render stats" << std::endl;
                    }
                }
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

void ScreenManager::initializeScreen(ScreenType screenType) {
    // If already created, do nothing
    if (activeScreens.count(screenType) && activeScreens[screenType]) return;
    auto it = screenFactories.find(screenType);
    if (it == screenFactories.end()) {
        std::cerr << "ERROR: No factory registered for screen type: " << static_cast<int>(screenType) << std::endl;
        return;
    }
    auto screen = it->second();
    screen->setScreenManager(this);
    if (!screen->initialize()) {
        std::cerr << "Failed to initialize screen: " << static_cast<int>(screenType) << std::endl;
        return;
    }
    activeScreens[screenType] = std::move(screen);
}

void ScreenManager::switchScreen(ScreenType screenType) {
    // Lazy initialize if needed
    initializeScreen(screenType);
    if (!activeScreens[screenType]) {
        std::cerr << "ERROR: Screen instance is null for type: " << static_cast<int>(screenType) << std::endl;
        return;
    }
    currentScreen = activeScreens[screenType].get();
}

void ScreenManager::cleanup() {
    // Clear screens first
    activeScreens.clear();
    
    // Clear other resources
    examples.reset();
    inputManager.reset();
    world.reset();
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