#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "Entity.h"
#include "Entities.h"
#include "GameState.h"

class InputManager {
public:
    InputManager(GLFWwindow* window, Camera& camera, Entities& entities, GameState& gameState);
    ~InputManager();

    // Set window after construction
    void setWindow(GLFWwindow* newWindow);

    void update(float deltaTime);
    void handleKeyInput(int key, int action);
    void handleMouseButton(int button, int action);
    void handleMouseMove(double x, double y);
    void handleScroll(double xoffset, double yoffset);

    // Configuration
    void loadConfig(const std::string& configPath);
    void setPanSpeed(float speed) { panSpeed = speed; }
    void setZoomSpeed(float speed) { zoomSpeed = speed; }
    void setEdgePanThreshold(float threshold) { edgePanThreshold = threshold; }
    void setEdgePanSpeed(float speed) { edgePanSpeed = speed; }
    void setInvertZoom(bool invert) { invertZoom = invert; }
    void setInvertPan(bool invert) { invertPan = invert; }

    // Static callback functions
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* window;
    Camera& camera;
    Entities& entities;
    GameState& gameState;
    
    // Camera control settings
    float panSpeed = 100.0f;
    float zoomSpeed = 1.0f;
    float edgePanThreshold = 0.05f; // 5% of screen width/height
    float edgePanSpeed = 50.0f;
    bool invertZoom = false;
    bool invertPan = false;

    // Mouse state
    glm::vec2 lastMousePos;
    glm::vec2 windowSize;
    bool isDragging = false;
    glm::vec2 dragStartPos;
    int selectedEntity = -1;

    // Key mappings
    std::unordered_map<std::string, int> keyMappings;

    void processKeyboardInput(float deltaTime);
    void processEdgePan(float deltaTime);
    void applyPan(const glm::vec2& direction, float speed, float deltaTime);
    void applyZoom(float amount);
    void handleEntitySelection(const glm::vec2& mousePos);
}; 