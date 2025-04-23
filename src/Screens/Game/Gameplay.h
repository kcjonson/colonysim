#pragma once

#include "../Screen.h"
#include <glm/glm.hpp>
#include <memory> // Required for std::unique_ptr

// Forward declarations
class Camera;
struct GLFWwindow;
class Interface; // Added forward declaration
class Entities;  // Added forward declaration
struct GameState;// Added forward declaration

class GameplayScreen : public Screen {
public:
    // Update constructor to accept Camera and Window pointers
    GameplayScreen(Camera* camera, GLFWwindow* window);
    ~GameplayScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;

private:
    // Store pointers to camera and window
    Camera* camera_ = nullptr;
    GLFWwindow* window_ = nullptr;
    GameState* gameState_ = nullptr; // Added GameState pointer

    // Gameplay specific components owned by this screen
    std::unique_ptr<Interface> interface_;
    std::unique_ptr<Entities> entities_;
};