#include "InputManager.h"
#include <fstream>
#include <iostream>
#include <json/json.h>

// Static member to store the instance pointer
static InputManager* s_instance = nullptr;

InputManager::InputManager(GLFWwindow* window, Camera& camera, EntityManager& entityManager)
    : window(window)
    , camera(camera)
    , entityManager(entityManager)
    , isDragging(false) {
    
    // Store instance pointer
    s_instance = this;
    
    // Get window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    windowSize = glm::vec2(static_cast<float>(width), static_cast<float>(height));
    
    // Get initial mouse position
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    lastMousePos = glm::vec2(static_cast<float>(x), static_cast<float>(y));
    
    // Set default key mappings
    keyMappings["pan_up"] = GLFW_KEY_W;
    keyMappings["pan_down"] = GLFW_KEY_S;
    keyMappings["pan_left"] = GLFW_KEY_A;
    keyMappings["pan_right"] = GLFW_KEY_D;
    
    // Set GLFW callbacks
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
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
    processEdgePan(deltaTime);
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
}

void InputManager::applyZoom(float amount) {
    camera.zoom(amount);
}

void InputManager::handleEntitySelection(const glm::vec2& mousePos) {
    // Convert mouse position to world coordinates
    glm::vec3 worldPos = camera.screenToWorld(glm::vec3(mousePos, 0.0f));
    
    // Check each entity for selection
    for (size_t i = 0; i < entityManager.getEntityCount(); ++i) {
        const Entity* entity = entityManager.getEntity(i);
        if (entity) {
            glm::vec2 entityPos = entity->getPosition();
            glm::vec2 entitySize = entity->getSize();
            
            // Check if mouse is within entity bounds
            if (worldPos.x >= entityPos.x - entitySize.x * 0.5f &&
                worldPos.x <= entityPos.x + entitySize.x * 0.5f &&
                worldPos.y >= entityPos.y - entitySize.y * 0.5f &&
                worldPos.y <= entityPos.y + entitySize.y * 0.5f) {
                
                selectedEntity = static_cast<int>(i);
                return;
            }
        }
    }
    
    // If no entity was selected
    selectedEntity = -1;
}

void InputManager::loadConfig(const std::string& configPath) {
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open input config file: " << configPath << std::endl;
        return;
    }
    
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(file, root)) {
        std::cerr << "Failed to parse input config file: " << reader.getFormattedErrorMessages() << std::endl;
        return;
    }
    
    // Load key mappings
    const Json::Value& keys = root["keys"];
    if (keys.isObject()) {
        for (const auto& key : keys.getMemberNames()) {
            keyMappings[key] = keys[key].asInt();
        }
    }
    
    // Load camera settings
    const Json::Value& camera = root["camera"];
    if (camera.isObject()) {
        if (camera.isMember("pan_speed")) panSpeed = camera["pan_speed"].asFloat();
        if (camera.isMember("zoom_speed")) zoomSpeed = camera["zoom_speed"].asFloat();
        if (camera.isMember("edge_pan_threshold")) edgePanThreshold = camera["edge_pan_threshold"].asFloat();
        if (camera.isMember("edge_pan_speed")) edgePanSpeed = camera["edge_pan_speed"].asFloat();
        if (camera.isMember("invert_zoom")) invertZoom = camera["invert_zoom"].asBool();
        if (camera.isMember("invert_pan")) invertPan = camera["invert_pan"].asBool();
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