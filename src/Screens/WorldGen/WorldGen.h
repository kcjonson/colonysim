#pragma once

#include "../Screen.h" // Changed from ../GameplayScreen.h to ../Screen.h
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Stars.h" // Added Stars class inclusion
#include "UI/WorldGenUI.h" // Updated path
#include "Generators/World.h"
#include "Generators/Generator.h"
#include "Renderers/World.h"
#include "Renderers/LandingLocation.h" // Add LandingLocation
#include "../Game/World.h" // Include for World class
#include "../../GameState.h" // Include for GameState
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

class WorldGenScreen : public Screen { // Changed from GameplayScreen to Screen
public:
    WorldGenScreen(Camera* camera, GLFWwindow* window);
    ~WorldGenScreen() override;
      bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput(float deltaTime) override;
    void onResize(int width, int height) override;
    
    // Keep isPointInRect helper method for UI detection
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    
    // Scroll handler
    void handleScroll(double xoffset, double yoffset);
    
    // Convert icosahedron world to terrain data
    void convertWorldToTerrainData();

private:
    // Terrain data
    std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> generatedTerrainData;
      // Store world parameters
    int worldWidth;
    int worldHeight;
    float waterLevel;
    bool worldGenerated;
      // Star background
    std::unique_ptr<WorldGen::Stars> stars;
      // 3D rendering properties
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    float cameraDistance;
    float rotationAngle;
    bool isDragging;
    
    // UI and input tracking
    float lastCursorX;
    float lastCursorY;
    
    // UI system
    std::unique_ptr<WorldGen::WorldGenUI> worldGenUI;
    
    // Store window pointer for callbacks
    GLFWwindow* window;    // New icosahedron-based world generator
    std::unique_ptr<WorldGen::Generators::World> world;
    std::unique_ptr<WorldGen::Renderers::World> worldRenderer;
    std::unique_ptr<WorldGen::Renderers::LandingLocation> landingLocation;
    WorldGen::PlanetParameters planetParams;
    uint64_t currentSeed = 12345; // Current seed for world generation
    float distortionFactor = 0.15f;
    
    // Progress tracking for world generation
    std::shared_ptr<WorldGen::ProgressTracker> progressTracker;
    
    // Static callback handling
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static std::unordered_map<GLFWwindow*, WorldGenScreen*> instances;    // Thread management
    std::thread generationThread;
    std::atomic<bool> isGenerating{false};
    std::atomic<bool> shouldStopGeneration{false};

    // Game world creation thread
    std::thread gameWorldThread;
    std::atomic<bool> isCreatingGameWorld{false};
    std::atomic<bool> shouldStopGameWorldCreation{false};
    std::unique_ptr<World> newWorld;
    
    // Game world creation parameters
    struct GameWorldCreationParams {
        float sampleRate;
        Camera* camera;
        GameState* gameState;
        GLFWwindow* window;
        std::string seed;
    };
    GameWorldCreationParams gameWorldParams;
    
    // Replace queue with single latest update
    struct ProgressMessage {
        float progress;
        std::string message;
        bool hasUpdate = false;  // Flag to indicate if there's a new update
    };
    ProgressMessage latestProgress;
    std::mutex progressMutex;
    
    // Thread worker methods
    void worldGenerationThreadFunc();
    void gameWorldCreationThreadFunc();
    
    // Process any pending progress messages
    void processProgressMessages();
};