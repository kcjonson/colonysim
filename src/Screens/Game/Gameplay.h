#pragma once

#include "../Screen.h"
#include <glm/glm.hpp>

// Forward declarations
class Camera;
struct GLFWwindow;

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
    
    // Any gameplay specific data
};