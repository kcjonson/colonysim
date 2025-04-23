#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Camera.h"
#include "GameState.h"

class InputManager {
public:
    InputManager(GLFWwindow* window, Camera& camera, GameState& gameState);
    ~InputManager();

    void update(float deltaTime);
    void handleKeyInput(int key, int action);
    void handleMouseButton(int button, int action);
    void handleMouseMove(double x, double y);
    void handleScroll(double xoffset, double yoffset);
    void handleCursorEnter(int entered); // Add new method for cursor enter/leave events

    // Configuration
    void loadConfig(const std::string& configPath);
    void setPanSpeed(float speed) { panSpeed = speed; }
    void setZoomSpeed(float speed) { zoomSpeed = speed; }
    void setEdgePanThreshold(float threshold) { edgePanThreshold = threshold; }
    void setEdgePanSpeed(float speed) { edgePanSpeed = speed; }
    void setInvertZoom(bool invert) { invertZoom = invert; }
    void setInvertPan(bool invert) { invertPan = invert; }
    void setMaxPanAcceleration(float accel) { maxPanAcceleration = accel; }
    void setPanAccelRate(float rate) { panAccelRate = rate; }
    void setMaxZoomAcceleration(float accel) { maxZoomAcceleration = accel; }
    void setZoomAccelRate(float rate) { zoomAccelRate = rate; }

    // Static callback functions
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorEnterCallback(GLFWwindow* window, int entered); // Add new static callback

private:
    GLFWwindow* window;
    Camera& camera;
    GameState& gameState;
    
    // Camera control settings
    float panSpeed = 100.0f;
    float zoomSpeed = 1.0f;  // Adjusted for better zoom sensitivity with new implementation
    float edgePanThreshold = 0.05f; // 5% of screen width/height
    float edgePanSpeed = 50.0f;
    bool invertZoom = false;
    bool invertPan = false;
    
    // Acceleration settings
    float maxPanAcceleration = 3.0f;    // Maximum acceleration multiplier
    float panAccelRate = 1.0f;          // How quickly acceleration builds
    float currentKeyAcceleration = 1.0f; // Current keyboard acceleration
    glm::vec2 lastKeyPanDirection = glm::vec2(0.0f);
    
    glm::vec2 lastEdgePanDirection = glm::vec2(0.0f);
    float currentEdgeAcceleration = 1.0f; // Current edge panning acceleration
    bool cursorInWindow = true;          // Flag to track if cursor is in window
    bool wasEdgePanning = false;         // Flag to track if we were edge panning last frame
    
    // Zoom settings
    float maxZoomAcceleration = 2.0f;     // Reduced maximum zoom acceleration (from 3.0f)
    float zoomAccelRate = 0.5f;           // Reduced rate of acceleration buildup (from 2.0f)
    float currentZoomAcceleration = 1.0f; // Current zoom acceleration
    float lastZoomDirection = 0.0f;       // Last zoom direction (positive = zoom in, negative = zoom out)

    // Mouse state
    glm::vec2 lastMousePos;
    glm::vec2 windowSize;
    bool isDragging = false;
    glm::vec2 dragStartPos;

    // Key mappings
    std::unordered_map<std::string, int> keyMappings;

    void processKeyboardInput(float deltaTime);
    void processEdgePan(float deltaTime);
    void applyPan(const glm::vec2& direction, float speed, float deltaTime);
    void applyZoom(float amount);
    
    // Helper function to check if cursor is in window
    bool isCursorInWindow();
    // Helper function to stop any ongoing edge panning
    void stopEdgePanning();
};