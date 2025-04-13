#include "InputManager.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Static member to store the instance pointer
static InputManager* s_instance = nullptr;

InputManager::InputManager(GLFWwindow* window, Camera& camera, Entities& entities, GameState& gameState)
    : window(window)
    , camera(camera)
    , entities(entities)
    , gameState(gameState)
    , isDragging(false) {
    
    // Store instance pointer
    s_instance = this;
    
    // Initialize with default window size and mouse position
    windowSize = glm::vec2(800.0f, 600.0f);
    lastMousePos = glm::vec2(0.0f, 0.0f);
    
    // If window is valid, update window size and mouse position
    if (window) {
        // Get window size
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        windowSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
        
        // Get initial mouse position
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        lastMousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
        
        // Set GLFW callbacks
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetCursorPosCallback(window, cursorPosCallback);
        glfwSetScrollCallback(window, scrollCallback);
    }
    
    // Set default key mappings
    keyMappings["pan_up"] = GLFW_KEY_W;
    keyMappings["pan_down"] = GLFW_KEY_S;
    keyMappings["pan_left"] = GLFW_KEY_A;
    keyMappings["pan_right"] = GLFW_KEY_D;
}

InputManager::~InputManager() {
    // Clear instance pointer
    s_instance = nullptr;
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (s_instance) {
        s_instance->handleKeyInput(key, action);
    }
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance) {
        s_instance->handleMouseButton(button, action);
    }
}

void InputManager::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        s_instance->handleMouseMove(xpos, ypos);
    }
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        s_instance->handleScroll(xoffset, yoffset);
    }
}

void InputManager::update(float deltaTime) {
    processKeyboardInput(deltaTime);
    
    // Only process edge pan if we have a valid window
    if (window) {
        processEdgePan(deltaTime);
    }
}

void InputManager::handleKeyInput(int key, int action) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        glm::vec2 panDirection(0.0f);
        
        if (key == keyMappings["pan_up"]) panDirection.y += 1.0f;
        if (key == keyMappings["pan_down"]) panDirection.y -= 1.0f;
        if (key == keyMappings["pan_left"]) panDirection.x -= 1.0f;
        if (key == keyMappings["pan_right"]) panDirection.x += 1.0f;
        
        if (glm::length(panDirection) > 0.0f) {
            panDirection = glm::normalize(panDirection);
            if (invertPan) panDirection = -panDirection;
            applyPan(panDirection, panSpeed, 1.0f / 60.0f);
        }
    }
}

void InputManager::handleMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            dragStartPos = lastMousePos;
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
            handleEntitySelection(lastMousePos);
        }
    }
}

void InputManager::handleMouseMove(double x, double y) {
    glm::vec2 currentPos(static_cast<float>(x), static_cast<float>(y));
    
    if (isDragging) {
        glm::vec2 delta = currentPos - lastMousePos;
        if (invertPan) delta = -delta;
        applyPan(delta, panSpeed * 0.1f, 1.0f / 60.0f);
    }

    // Format window position as "x, y" with no decimals
    gameState.set("input.windowPos", 
                 std::to_string((int)currentPos.x) + ", " + std::to_string((int)currentPos.y));
    
    // Format world position with 1 decimal place
    glm::vec2 worldPos = glm::vec2(camera.screenToWorld(glm::vec3(currentPos, 0.0f)));
    char worldPosStr[50];
    snprintf(worldPosStr, sizeof(worldPosStr), "%.1f, %.1f", worldPos.x, worldPos.y);
    gameState.set("input.worldPos", worldPosStr);

    lastMousePos = currentPos;
}

void InputManager::handleScroll(double xoffset, double yoffset) {
    float zoomAmount = static_cast<float>(yoffset) * zoomSpeed;
    if (invertZoom) zoomAmount = -zoomAmount;
    applyZoom(zoomAmount);
}

void InputManager::processKeyboardInput(float deltaTime) {
    // Keyboard input is now handled in the callback
}

void InputManager::processEdgePan(float deltaTime) {
    // Skip if window is null
    if (!window) {
        return;
    }
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    glm::vec2 mousePos = lastMousePos;
    glm::vec2 panDirection(0.0f);
    
    float edgeThreshold = edgePanThreshold * static_cast<float>(width);
    
    if (mousePos.x < edgeThreshold) panDirection.x -= 1.0f;
    if (mousePos.x > width - edgeThreshold) panDirection.x += 1.0f;
    if (mousePos.y < edgeThreshold) panDirection.y -= 1.0f;
    if (mousePos.y > height - edgeThreshold) panDirection.y += 1.0f;
    
    if (glm::length(panDirection) > 0.0f) {
        panDirection = glm::normalize(panDirection);
        if (invertPan) panDirection = -panDirection;
        applyPan(panDirection, edgePanSpeed, deltaTime);
    }
}

void InputManager::applyPan(const glm::vec2& direction, float speed, float deltaTime) {
    glm::vec3 offset(direction.x * speed * deltaTime, direction.y * speed * deltaTime, 0.0f);
    camera.move(offset);

    // Log the camera's new position
    glm::vec3 cameraPos = camera.getPosition();
    char posStr[50];
    snprintf(posStr, sizeof(posStr), "%.1f, %.1f", cameraPos.x, cameraPos.y);
    gameState.set("camera.position", posStr);
}

void InputManager::applyZoom(float amount) {
    camera.zoom(amount);
}

void InputManager::handleEntitySelection(const glm::vec2& mousePos) {
    // Convert mouse position to world coordinates
    glm::vec3 worldPos = camera.screenToWorld(glm::vec3(mousePos, 0.0f));
    
    // Check each entity for selection
    for (size_t i = 0; i < entities.getEntityCount(); ++i) {
        const Entity* entity = entities.getEntity(i);
        if (entity) {
            glm::vec2 entityPos = entity->getPosition();
            glm::vec2 entitySize = entity->getSize();
            
            // Check if mouse is within entity bounds
            if (worldPos.x >= entityPos.x - entitySize.x * 0.5f &&
                worldPos.x <= entityPos.x + entitySize.x * 0.5f &&
                worldPos.y >= entityPos.y - entitySize.y * 0.5f &&
                worldPos.y <= entityPos.y + entitySize.y * 0.5f) {
                
                // Entity found, store its index
                selectedEntity = static_cast<int>(i);
                gameState.set("input.selectedEntity", std::to_string(selectedEntity));
                
                // Update entity state to selected
                Entity* selectedEntity = entities.getEntity(i);
                if (selectedEntity) {
                    // Apply selection effect
                    std::cout << "Selected entity " << i << std::endl;
                }
                
                return;
            }
        }
    }
    
    // No entity selected
    selectedEntity = -1;
    gameState.set("input.selectedEntity", "-1");
}

// glm::vec2 InputManager::getCursorWorldPos() {
//     return camera.screenToWorld(glm::vec3(lastMousePos, 0.0f));
// }

void InputManager::loadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open input config file: " << configPath << std::endl;
        return;
    }
    
    try {
        json root = json::parse(file);
        
        // Load key mappings
        if (root.contains("keys") && root["keys"].is_object()) {
            for (auto& [key, value] : root["keys"].items()) {
                keyMappings[key] = value.get<int>();
            }
        }
        
        // Load camera settings
        if (root.contains("camera") && root["camera"].is_object()) {
            auto& camera = root["camera"];
            
            if (camera.contains("panSpeed")) 
                panSpeed = camera["panSpeed"].get<float>();
            
            if (camera.contains("zoomSpeed")) 
                zoomSpeed = camera["zoomSpeed"].get<float>();
            
            if (camera.contains("edgePanThreshold")) 
                edgePanThreshold = camera["edgePanThreshold"].get<float>();
            
            if (camera.contains("edgePanSpeed")) 
                edgePanSpeed = camera["edgePanSpeed"].get<float>();
            
            if (camera.contains("invertZoom")) 
                invertZoom = camera["invertZoom"].get<bool>();
            
            if (camera.contains("invertPan")) 
                invertPan = camera["invertPan"].get<bool>();
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "Failed to parse input config file: " << e.what() << std::endl;
    }
}

void InputManager::setWindow(GLFWwindow* newWindow) {
    window = newWindow;
    
    // Get window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    windowSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
    
    // Get initial mouse position
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    lastMousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
    
    // Set GLFW callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
} 