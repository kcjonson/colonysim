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

// Constructor that accepts an optional GameState pointer
ScreenManager::ScreenManager(GameState* initializedGameState)
    : window(nullptr)
    , currentScreen(nullptr)
    , isRunning(true) {
    // If an external GameState was provided, use it
    if (initializedGameState) {
        gameState = std::unique_ptr<GameState>(initializedGameState);
        std::cout << "Using externally initialized GameState" << std::endl;
    }
    // Otherwise, the initialize() method will create one
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

    // Create shared game state only if one wasn't provided in constructor
    if (!gameState) {
        gameState = std::make_unique<GameState>();
        if (!gameState) {
             std::cerr << "ERROR: Failed to create GameState" << std::endl;
             return false;
        }
        std::cout << "Created new GameState in ScreenManager::initialize()" << std::endl;
    } else {
        std::cout << "Using existing GameState provided to constructor" << std::endl;
    }

    // *** Create all screens now that camera and window are ready ***
    try {
        // Pass camera and window pointers to constructors where needed
        screens[ScreenType::Splash] = std::make_unique<SplashScreen>(camera.get(), window); // Pass camera and window
        screens[ScreenType::MainMenu] = std::make_unique<MainMenuScreen>(camera.get(), window);
        screens[ScreenType::WorldGen] = std::make_unique<WorldGenScreen>(camera.get(), window);
        // Update GameplayScreen creation to pass camera and window
        screens[ScreenType::Gameplay] = std::make_unique<GameplayScreen>(camera.get(), window); 
        screens[ScreenType::Settings] = std::make_unique<SettingsScreen>(camera.get(), window);
        screens[ScreenType::Developer] = std::make_unique<DeveloperScreen>(camera.get(), window);
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to create screens: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "ERROR: Unknown exception when creating screens" << std::endl;
        return false;
    }

    // *** Initialize all screens ***
    for (auto& [type, screen] : screens) {
        if (!screen) {
            std::cerr << "ERROR: Screen is null for type: " << static_cast<int>(type) << std::endl;
            return false; // Fail initialization if a screen is null
        }
        screen->setScreenManager(this); // Set manager reference
        if (!screen->initialize()) {
            std::cerr << "Failed to initialize screen: " << static_cast<int>(type) << std::endl;
            return false;
        }
    }

    // Set current screen to splash screen
    currentScreen = screens[ScreenType::Splash].get();
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

    // Make sure we have a valid GameState before proceeding
    if (!gameState) {
        // If no GameState exists yet, create a temporary one that will be replaced later
        std::cout << "WARNING: No GameState available in initializeOpenGL(), creating a temporary one" << std::endl;
        gameState = std::make_unique<GameState>();
        if (!gameState) {
            std::cerr << "ERROR: Failed to create temporary GameState" << std::endl;
            return false;
        }
    }
    
    // Pre-initialize some key GameState values to prevent access violations
    try {
        gameState->set("system.version", "0.1.0");
        gameState->set("system.fps", "0");
        gameState->set("world.totalTiles", "0");
        gameState->set("world.shownTiles", "0");
        gameState->set("world.totalShapes", "0");
        gameState->set("world.tileMemKB", "0");
        gameState->set("world.shapeMemKB", "0");
        gameState->set("world.totalMemKB", "0");
        gameState->set("input.windowPos", "0, 0");
        gameState->set("input.worldPos", "0, 0");
        gameState->set("camera.position", "0, 0");
        gameState->set("rend.vertices", "0");
        gameState->set("rend.indices", "0");
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception when initializing GameState values: " << e.what() << std::endl;
    }
    
    // Initialize interface (pass gameState, camera, and window)
    try {
        interface = std::make_unique<Interface>(*gameState, camera.get(), window);
        if (!interface) {
            std::cerr << "ERROR: Failed to create Interface" << std::endl;
            return false;
        }
        
        if (!interface->initialize()) {
            std::cerr << "ERROR: Interface initialization failed!" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Interface initialization: " << e.what() << std::endl;
        return false;
    }
    
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
    
    // Initialize entities (pass camera and window)
    try {
        entities = std::make_unique<Entities>(camera.get(), window);
        if (!entities) {
            std::cerr << "ERROR: Failed to create Entities" << std::endl;
            return false;
        }
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Exception during Entities initialization: " << e.what() << std::endl;
        return false;
    }
    
    // Initialize input manager
    try {
        // Pass camera reference, window pointer, and other dependencies
        inputManager = std::make_unique<InputManager>(window, *camera, *entities, *gameState);
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

void ScreenManager::switchScreen(ScreenType screenType) {
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