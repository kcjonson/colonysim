#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include "Screen.h"

// Forward declarations
struct GLFWwindow;  // Changed from class to struct to maintain consistency across the codebase
class Camera;
struct GameState;   // Changed to struct to match GameState.h
class World;
// Removed: class Entities;
class InputManager;
// Removed: class Interface;
class Examples;

// Screen types enum
enum class ScreenType {
    Splash,
    MainMenu,
    WorldGen,
    Gameplay,
    Settings,
    Developer
};

class ScreenManager {
public:
    // Constructor that requires GameState pointer (no default parameter)
    ScreenManager(GameState* initializedGameState);
    ~ScreenManager();

    bool initialize();
    void run();
    void switchScreen(ScreenType screenType);
    void cleanup();
    
    // For initializing OpenGL after splash screen
    bool initializeOpenGL();

    // Getters for shared resources
    GLFWwindow* getWindow() const { return window; }
    Camera* getCamera() const { return camera.get(); }
    GameState* getGameState() const { return gameState.get(); }
    World* getWorld() const { return world.get(); }
    // Removed: Entities* getEntities() const { return entities.get(); }
    InputManager* getInputManager() const { return inputManager.get(); }
    // Removed: Interface* getInterface() const { return interface.get(); }
    Examples* getExamples() const { return examples.get(); }

    // Window callback handlers
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);

    void initializeScreen(ScreenType screenType); // Manually initialize a screen (preload)

private:
    // Game shared resources
    GLFWwindow* window;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<GameState> gameState;
    std::unique_ptr<World> world;
    // Removed: std::unique_ptr<Entities> entities;
    std::unique_ptr<InputManager> inputManager;
    // Removed: std::unique_ptr<Interface> interface;
    std::unique_ptr<Examples> examples;
    
    // Screen management (lazy init)
    std::unordered_map<ScreenType, std::function<std::unique_ptr<Screen>()>> screenFactories;
    std::unordered_map<ScreenType, std::unique_ptr<Screen>> activeScreens;
    Screen* currentScreen;
    bool isRunning;
    
    // OpenGL initialization state
    bool openglInitialized;
};