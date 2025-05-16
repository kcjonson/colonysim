#pragma once

#include "../Screen.h"
#include <glm/glm.hpp>
#include <memory>

// Forward declarations
class Camera;
class World;
class Entities;
class InputManager;
class Interface;
class Examples;
struct GLFWwindow;
struct GameState;

class GameScreen : public Screen {
public:
    GameScreen(Camera* camera, GLFWwindow* window);
    ~GameScreen() override;    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput(float deltaTime) override;
    void onResize(int width, int height) override;

private:
    void processInput();
    
    // Static callback for GLFW
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    
    // Pointers to shared resources from ScreenManager
    Camera* camera_;
    GLFWwindow* window_;
    GameState* gameState_;

    // Game components owned by this screen
    std::unique_ptr<Interface> interface_;
    std::unique_ptr<Entities> entities_;
    
    // For render stats logging
    float timeSinceLastRenderLog;
    
    // Game state
    bool isRunning;
};