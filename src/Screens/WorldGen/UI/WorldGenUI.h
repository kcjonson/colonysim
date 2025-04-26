#pragma once

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>
#include "../../../Rendering/Layer.h"
#include "../../../Rendering/Shapes/Rectangle.h"
#include "../../../Rendering/Shapes/Text.h"
#include "../Core/TerrainGenerator.h" // Updated path to Core directory
#include "../../MainMenu/MainMenu.h" // Updated path to MainMenu.h

struct GLFWwindow;
class Camera;

namespace WorldGen {

// UI Event types
enum class UIEvent {
    GenerateWorld,
    GoToLand,
    Back
};

// Callback definition
using UIEventCallback = std::function<void()>;

enum class UIState {
    ParameterSetup,  // Initial state: adjusting world generation parameters
    Generating,      // While world is being generated
    Viewing,         // Examining the generated world
    Saving,          // Saving world parameters
    Loading          // Loading saved parameters
};

class WorldGenUI {
public:
    WorldGenUI(Camera* camera, GLFWwindow* window);
    ~WorldGenUI();

    // Initialize UI resources
    bool initialize();
    
    // Register event listeners
    void addEventListener(UIEvent event, UIEventCallback callback);
    
    // UI Layout and rendering
    void layoutUI(int windowWidth, int windowHeight, int worldWidth, int worldHeight, 
                  float waterLevel, int seed, bool worldGenerated);
    
    // State management
    UIState getState() const { return state; }
    void setState(UIState newState);
    
    // Accessors
    float getSidebarWidth() const { return sidebarWidth; }
    
    // Event handling
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    void handleButtonClicks(float mouseX, float mouseY, bool isPressed, bool wasPressed);
    
    // Get all layers for rendering
    std::vector<std::shared_ptr<Rendering::Layer>> getAllLayers() const;
    
    // Progress tracking
    void updateProgress(float progress, const std::string& statusMessage);

private:
    // Create button internal - now private as WorldGenScreen shouldn't call this directly
    void createButton(const std::string& text, const glm::vec4& color, 
                      const glm::vec4& hoverColor, const std::function<void()>& callback);
                      
    // Helper methods for UI states
    void setupParameterUI(int windowWidth, int windowHeight, int worldWidth, int worldHeight, 
                          float waterLevel, int seed);
    void setupGeneratingUI(int windowWidth, int windowHeight);
    void setupViewingUI(int windowWidth, int windowHeight);
    
    // Initialize buttons
    void initializeButtons();

    // UI elements
    std::vector<MenuButton> buttons;
    float sidebarWidth;
    
    // Event callbacks
    std::unordered_map<UIEvent, UIEventCallback> eventHandlers;
    
    // Progress tracking
    float currentProgress;
    std::string statusMessage;
    
    // UI layers
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> previewLayer;
    std::shared_ptr<Rendering::Layer> controlsLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
    std::shared_ptr<Rendering::Layer> sidebarLayer;
    
    // Camera and window references
    Camera* camera;
    GLFWwindow* window;
    
    // UI state
    UIState state;
};

} // namespace WorldGen