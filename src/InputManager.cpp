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
    // Debug output for scroll events
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

    // Check if the gameState is valid before attempting to access it
    if (&gameState != nullptr) {
        // Update GameState once per frame with latest values
        try {
            // Window Position (from last mouse move)
            gameState.set("input.windowPos", 
                        std::to_string((int)lastMousePos.x) + ", " + std::to_string((int)lastMousePos.y));
            
            // World Position (from last mouse move)
            // Only calculate screenToWorld once here
            glm::vec2 worldPos = glm::vec2(camera.screenToWorld(glm::vec3(lastMousePos, 0.0f)));
            char worldPosStr[50];
            snprintf(worldPosStr, sizeof(worldPosStr), "%.1f, %.1f", worldPos.x, worldPos.y);
            gameState.set("input.worldPos", worldPosStr);

            // Camera Position (from last pan/zoom)
            glm::vec3 cameraPos = camera.getPosition();
            char cameraPosStr[50];
            snprintf(cameraPosStr, sizeof(cameraPosStr), "%.1f, %.1f", cameraPos.x, cameraPos.y);
            gameState.set("camera.position", cameraPosStr);
        }
        catch (const std::exception& e) {
            std::cerr << "Error updating GameState in InputManager: " << e.what() << std::endl;
        }
    }
    else {
        std::cerr << "Warning: GameState reference is invalid in InputManager::update" << std::endl;
    }

    // Selected Entity (updated in handleMouseButton)
    // No change needed here, it's updated on click
}

void InputManager::handleKeyInput(int key, int action) {
    // We now handle keyboard panning in processKeyboardInput for continuous acceleration
    // This callback can still be used for one-shot key actions if needed
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
    if (yoffset == 0) return;
    
    // Convert scroll input to zoom amount
    // Positive yoffset = scroll up = zoom in
    // Negative yoffset = scroll down = zoom out
    float zoomAmount = static_cast<float>(yoffset) * zoomSpeed;
    
    // Apply inversion if needed (user preference)
    if (invertZoom) {
        zoomAmount = -zoomAmount;
    }
    
    // Debug output for zoom events
    std::cout << "Zoom event: " << yoffset << " (inverted: " << invertZoom << ")" << std::endl;
    std::cout << "Zoom amount: " << zoomAmount << std::endl;

    
    // Pass the zoom amount to the camera
    camera.zoom(zoomAmount);
}

void InputManager::processKeyboardInput(float deltaTime) {
    // Check for keyboard pan input
    glm::vec2 panDirection(0.0f);
    
    if (glfwGetKey(window, keyMappings["pan_up"]) == GLFW_PRESS) panDirection.y += 1.0f;
    if (glfwGetKey(window, keyMappings["pan_down"]) == GLFW_PRESS) panDirection.y -= 1.0f;
    if (glfwGetKey(window, keyMappings["pan_left"]) == GLFW_PRESS) panDirection.x -= 1.0f;
    if (glfwGetKey(window, keyMappings["pan_right"]) == GLFW_PRESS) panDirection.x += 1.0f;
    
    if (glm::length(panDirection) > 0.0f) {
        // Normalize for consistent movement in all directions
        panDirection = glm::normalize(panDirection);
        
        // Apply inversion if needed
        if (invertPan) panDirection = -panDirection;
        
        // If continuing in same direction, increase acceleration
        if (glm::dot(panDirection, lastKeyPanDirection) > 0.7f) {
            currentKeyAcceleration = glm::min(currentKeyAcceleration + panAccelRate * deltaTime, maxPanAcceleration);
        } else {
            // Reset acceleration when changing direction
            currentKeyAcceleration = 1.0f;
        }
        
        // Save direction for next frame
        lastKeyPanDirection = panDirection;
        
        // Apply pan with acceleration
        applyPan(panDirection, panSpeed * currentKeyAcceleration, deltaTime);
    } else {
        // No keys pressed, reset acceleration
        currentKeyAcceleration = 1.0f;
        lastKeyPanDirection = glm::vec2(0.0f);
    }
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
    
    if (mousePos.x < edgeThreshold) panDirection.x += 1.0f;
    if (mousePos.x > width - edgeThreshold) panDirection.x -= 1.0f;
    if (mousePos.y < edgeThreshold) panDirection.y += 1.0f;
    if (mousePos.y > height - edgeThreshold) panDirection.y -= 1.0f;
    
    if (glm::length(panDirection) > 0.0f) {
        panDirection = glm::normalize(panDirection);
        if (invertPan) panDirection = -panDirection;
        
        // If continuing in same direction, increase acceleration
        if (glm::dot(panDirection, lastEdgePanDirection) > 0.7f) {
            currentEdgeAcceleration = glm::min(currentEdgeAcceleration + panAccelRate * deltaTime, maxPanAcceleration);
        } else {
            // Reset acceleration when changing direction
            currentEdgeAcceleration = 1.0f;
        }
        
        // Save direction for next frame
        lastEdgePanDirection = panDirection;
        
        // Apply pan with acceleration
        applyPan(panDirection, edgePanSpeed * currentEdgeAcceleration, deltaTime);
    } else {
        // Not edge panning, reset acceleration
        currentEdgeAcceleration = 1.0f;
        lastEdgePanDirection = glm::vec2(0.0f);
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
    
    // // Check each entity for selection
    // for (size_t i = 0; i < entities.getEntityCount(); ++i) {
    //     const Entity* entity = entities.getEntity(i);
    //     if (entity) {
    //         glm::vec2 entityPos = entity->getPosition();
    //         glm::vec2 entitySize = entity->getSize();
            
    //         // Check if mouse is within entity bounds
    //         if (worldPos.x >= entityPos.x - entitySize.x * 0.5f &&
    //             worldPos.x <= entityPos.x + entitySize.x * 0.5f &&
    //             worldPos.y >= entityPos.y - entitySize.y * 0.5f &&
    //             worldPos.y <= entityPos.y + entitySize.y * 0.5f) {
                
    //             // Entity found, store its index
    //             selectedEntity = static_cast<int>(i);
    //             gameState.set("input.selectedEntity", std::to_string(selectedEntity));
                
    //             // Update entity state to selected
    //             Entity* selectedEntity = entities.getEntity(i);
    //             if (selectedEntity) {
    //                 // Apply selection effect
    //                 std::cout << "Selected entity " << i << std::endl;
    //             }
                
    //             return;
    //         }
    //     }
    // }
    
    // // No entity selected
    // selectedEntity = -1;
    // gameState.set("input.selectedEntity", "-1");
}

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
                
            // Load acceleration settings
            if (camera.contains("maxPanAcceleration"))
                maxPanAcceleration = camera["maxPanAcceleration"].get<float>();
                
            if (camera.contains("panAccelRate"))
                panAccelRate = camera["panAccelRate"].get<float>();
                
            // Load zoom acceleration settings
            if (camera.contains("maxZoomAcceleration"))
                maxZoomAcceleration = camera["maxZoomAcceleration"].get<float>();
                
            if (camera.contains("zoomAccelRate"))
                zoomAccelRate = camera["zoomAccelRate"].get<float>();
        }
    }
    catch (const json::parse_error& e) {
        std::cerr << "Failed to parse input config file: " << e.what() << std::endl;
    }
}