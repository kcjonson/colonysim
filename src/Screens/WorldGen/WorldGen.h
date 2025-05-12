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

class WorldGenScreen : public Screen { // Changed from GameplayScreen to Screen
public:
    WorldGenScreen(Camera* camera, GLFWwindow* window);
    ~WorldGenScreen() override;
    
    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;
    
    // Keep isPointInRect helper method for UI detection
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    
    // Scroll handler
    void handleScroll(double xoffset, double yoffset);

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
    GLFWwindow* m_window;
    
    // Static callback handling
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static std::unordered_map<GLFWwindow*, WorldGenScreen*> s_instances;
};