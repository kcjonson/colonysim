#include "InputManager.h"
#include "Rendering/Components/Form/Text.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// Static member to store the instance pointer
static InputManager* s_instance = nullptr;

// Removed Entities& entities parameter
InputManager::InputManager(GLFWwindow* window, Camera& camera, GameState& gameState)
    : window(window)
    , camera(camera)
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
        glfwSetCursorEnterCallback(window, cursorEnterCallback);
        glfwSetCharCallback(window, charCallback); // Add character callback
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

void InputManager::cursorEnterCallback(GLFWwindow* window, int entered) {
    if (s_instance) {
        s_instance->handleCursorEnter(entered);
    }
}

void InputManager::charCallback(GLFWwindow* window, unsigned int codepoint) {
    if (s_instance) {
        s_instance->handleCharInput(codepoint);
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
    
    // Forward keypress to the currently focused text input component
    auto focusedText = Rendering::Components::Form::Text::focusedTextInput;
    if (focusedText) {
        focusedText->handleKeyInput(key, 0, action, 0);
    }
}

void InputManager::handleMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            dragStartPos = lastMousePos;
        } else if (action == GLFW_RELEASE) {
            isDragging = false;
            // Entity selection logic should now be handled elsewhere (e.g., GameplayScreen)
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
    // std::cout << "Zoom event: " << yoffset << " (inverted: " << invertZoom << ")" << std::endl;
    // std::cout << "Zoom amount: " << zoomAmount << std::endl;

    
    // Pass the zoom amount to the camera
    camera.zoom(zoomAmount);
}

void InputManager::handleCursorEnter(int entered) {
    // std::cout << "Cursor " << (entered ? "entered" : "left") << " window" << std::endl;
    
    // Update cursor state - this is a reliable way to know when cursor leaves the window
    cursorInWindow = entered != 0;
    
    // If cursor left the window and was edge panning, stop it
    if (!cursorInWindow && wasEdgePanning) {
        // std::cout << "Cursor left window while edge panning - stopping pan" << std::endl;
        stopEdgePanning();
        // Force camera to stop by explicitly setting position
        glm::vec3 currentPos = camera.getPosition();
        camera.setPosition(currentPos);
    }
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

bool InputManager::isCursorInWindow() {
    if (!window) {
        return false;
    }
    
    // Check if window has focus
    int windowFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED);
    if (!windowFocused) {
        // std::cout << "Window lost focus" << std::endl;
        return false;
    }
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    bool inWindow = (lastMousePos.x >= 0 && lastMousePos.x <= width && 
                    lastMousePos.y >= 0 && lastMousePos.y <= height);
                    
    if (!inWindow) {
        // std::cout << "Cursor outside window bounds: " << lastMousePos.x << ", " << lastMousePos.y << 
        //          " (window: " << width << "x" << height << ")" << std::endl;
    }
    
    return inWindow;
}

void InputManager::stopEdgePanning() {
    // std::cout << "Stopping edge panning" << std::endl;
    // Reset acceleration and direction
    currentEdgeAcceleration = 1.0f;
    lastEdgePanDirection = glm::vec2(0.0f);
    wasEdgePanning = false;
}

void InputManager::processEdgePan(float deltaTime) {
    // Skip if window is null
    if (!window) {
        return;
    }
    
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // First check if cursor is outside window via the cursor enter callback state
    // This is more reliable than position-based checks
    if (!cursorInWindow) {
        // If we were edge panning, force a stop of camera movement
        if (wasEdgePanning) {
            // std::cout << "Cursor outside window, forcing camera to stop" << std::endl;
            wasEdgePanning = false;
            currentEdgeAcceleration = 1.0f;
            lastEdgePanDirection = glm::vec2(0.0f);
            
            // Explicitly set camera position to current position to stop movement
            glm::vec3 currentPos = camera.getPosition();
            camera.setPosition(currentPos);
        }
        return;
    }
    
    glm::vec2 mousePos = lastMousePos;
    glm::vec2 panDirection(0.0f);
    
    float edgeThreshold = edgePanThreshold * static_cast<float>(width);
    
    // Fixing left/right edge panning direction - reversing the X-axis values
    if (mousePos.x < edgeThreshold) panDirection.x -= 1.0f; // Changed from += to -=
    if (mousePos.x > width - edgeThreshold) panDirection.x += 1.0f; // Changed from -= to +=
    if (mousePos.y < edgeThreshold) panDirection.y += 1.0f;
    if (mousePos.y > height - edgeThreshold) panDirection.y -= 1.0f;
    
    if (glm::length(panDirection) > 0.0f) {
        wasEdgePanning = true; // Mark that we're edge panning
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
        if (wasEdgePanning) {
            wasEdgePanning = false;
        }
        currentEdgeAcceleration = 1.0f;
        lastEdgePanDirection = glm::vec2(0.0f);
    }
}

void InputManager::applyPan(const glm::vec2& direction, float speed, float deltaTime) {
    // Prevent any panning when cursor is outside the window during edge panning
    // This acts as a safety mechanism in case other checks have failed
    if (wasEdgePanning && !cursorInWindow) {
        // std::cout << "applyPan blocked: cursor outside window" << std::endl;
        return;
    }
    
    glm::vec3 offset(direction.x * speed * deltaTime, direction.y * speed * deltaTime, 0.0f);
    
    // Debug output for pan events
    // if (glm::length(offset) > 0.0f) {
    //     std::cout << "applyPan: dir=(" << direction.x << "," << direction.y << 
    //              "), speed=" << speed << 
    //              ", offset=(" << offset.x << "," << offset.y << 
    //              "), cursorInWindow=" << (cursorInWindow ? "true" : "false") << 
    //              ", wasEdgePanning=" << (wasEdgePanning ? "true" : "false") << std::endl;
    // }
    
    camera.move(offset);
}

void InputManager::applyZoom(float amount) {
    camera.zoom(amount);
}

// Removed handleEntitySelection method implementation

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

void InputManager::handleCharInput(unsigned int codepoint) {
    // Forward character input to the currently focused text input component
    auto focusedText = Rendering::Components::Form::Text::focusedTextInput;
    if (focusedText) {
        focusedText->handleCharInput(codepoint);
    }
}