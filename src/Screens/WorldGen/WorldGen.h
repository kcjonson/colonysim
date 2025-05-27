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
    unsigned int seed;
    bool worldGenerated;
      // Star background
    std::unique_ptr<WorldGen::Stars> m_stars;
      // 3D rendering properties
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_cameraDistance;
    float m_rotationAngle;
    bool m_isDragging;
    
    // UI and input tracking
    float lastCursorX;
    float lastCursorY;
    
    // UI system
    std::unique_ptr<WorldGen::WorldGenUI> m_worldGenUI;
    
    // Store window pointer for callbacks
    GLFWwindow* m_window;    // New icosahedron-based world generator
    std::unique_ptr<WorldGen::Generators::World> m_world;
    std::unique_ptr<WorldGen::Renderers::World> m_worldRenderer;
    std::unique_ptr<WorldGen::Renderers::LandingLocation> m_landingLocation;
    WorldGen::PlanetParameters m_planetParams;
    float m_distortionFactor = 0.15f;
    
    // Progress tracking for world generation
    std::shared_ptr<WorldGen::ProgressTracker> m_progressTracker;
    
    // Static callback handling
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static std::unordered_map<GLFWwindow*, WorldGenScreen*> s_instances;    // Thread management
    std::thread m_generationThread;
    std::atomic<bool> m_isGenerating{false};
    std::atomic<bool> m_shouldStopGeneration{false};

    // Game world creation thread
    std::thread m_gameWorldThread;
    std::atomic<bool> m_isCreatingGameWorld{false};
    std::atomic<bool> m_shouldStopGameWorldCreation{false};
    std::unique_ptr<World> m_newWorld;
    
    // Game world creation parameters
    struct GameWorldCreationParams {
        float sampleRate;
        Camera* camera;
        GameState* gameState;
        GLFWwindow* window;
        std::string seed;
    };
    GameWorldCreationParams m_gameWorldParams;
    
    // Replace queue with single latest update
    struct ProgressMessage {
        float progress;
        std::string message;
        bool hasUpdate = false;  // Flag to indicate if there's a new update
    };
    ProgressMessage m_latestProgress;
    std::mutex m_progressMutex;
    
    // Thread worker methods
    void worldGenerationThreadFunc();
    void gameWorldCreationThreadFunc();
    
    // Process any pending progress messages
    void processProgressMessages();
};