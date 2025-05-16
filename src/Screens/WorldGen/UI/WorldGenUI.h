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
#include "../Core/WorldGenParameters.h" // Updated path to Core directory

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
    void onResize(int windowWidth, int windowHeight);
    
    // State management
    void setState(UIState newState);
    void setPlanetParameters(const PlanetParameters& params);
   
    // Accessors
    // Used to center the world in the preview area
    float getSidebarWidth() const { return sidebarWidth; }

    // Render and update methods for main game loop
    void render();
    void update(float deltaTime);
    void handleInput();
    
    // Progress tracking (setter)
    void setProgress(float progress, const std::string& statusMessage);

private:
    float sidebarWidth;
    std::tuple <int, int> windowSize;
    
    // Event callbacks
    std::unordered_map<UIEvent, UIEventCallback> eventHandlers;
    
    // Progress tracking
    float currentProgress;
    std::string statusMessage;    // UI elements that are reused between states
    std::shared_ptr<Rendering::Shapes::Text> titleText;
    std::shared_ptr<Rendering::Shapes::Text> statusText;
    
    // Parameter UI elements
    std::shared_ptr<Rendering::Shapes::Text> radiusLabel;
    std::shared_ptr<Rendering::Shapes::Text> radiusValue;
    std::shared_ptr<Rendering::Shapes::Text> massLabel;
    std::shared_ptr<Rendering::Shapes::Text> massValue;
    std::shared_ptr<Rendering::Shapes::Text> waterLabel;
    std::shared_ptr<Rendering::Shapes::Text> waterValue;
    std::shared_ptr<Rendering::Shapes::Text> seedLabel;
    std::shared_ptr<Rendering::Shapes::Text> seedValue;
    
    // Progress UI elements
    std::shared_ptr<Rendering::Shapes::Rectangle> progressBackground;
    std::shared_ptr<Rendering::Shapes::Rectangle> progressFill;
    std::shared_ptr<Rendering::Shapes::Text> progressText;
    
    // Preview area elements
    std::shared_ptr<Rendering::Shapes::Text> generatingMessage;
    std::shared_ptr<Rendering::Shapes::Text> pleaseWaitMessage;
    
    // UI layers
    std::shared_ptr<Rendering::Layer> sidebarLayer;
    std::shared_ptr<Rendering::Layer> infoLayer;
    
    // Camera and window references
    Camera* camera;
    GLFWwindow* window;
    
    // UI state
    UIState state;
};

} // namespace WorldGen