#pragma once

#include "../Screen.h" // Changed from ../GameplayScreen.h to ../Screen.h
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>
#include "../../Rendering/Layer.h" // Updated path to Layer.h
#include "../../Rendering/Shapes/Rectangle.h" // Updated path to Rectangle.h
#include "UI/WorldGenUI.h" // Updated path
#include "Lithosphere/Plate/TectonicPlate.h" // Updated path
#include "Lithosphere/Plate/PlateGenerator.h" // Updated path
#include "Renderers/PlateRenderer.h" // Updated path
#include "Renderers/GlobeRenderer.h" // Updated path from Planet/GlobeRenderer.h

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
    
    // Star background layer
    std::shared_ptr<Rendering::Layer> starLayer;
    void renderStars(int width, int height);
    
    // 3D rendering properties
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_cameraDistance;
    float m_rotationAngle;
    bool m_isDragging;
    
    // UI and input tracking
    float lastCursorX;
    float lastCursorY;
    
    // Tectonic plate system
    bool m_platesGenerated;
    std::vector<std::shared_ptr<WorldGen::TectonicPlate>> m_plates;
    std::vector<glm::vec3> m_planetVertices;
    std::vector<unsigned int> m_planetIndices;
    
    // UI system
    std::unique_ptr<WorldGen::WorldGenUI> m_worldGenUI;
    
    // Planet generation components 
    std::unique_ptr<WorldGen::PlateGenerator> m_plateGenerator;
    std::unique_ptr<WorldGen::PlateRenderer> m_plateRenderer;
    std::unique_ptr<WorldGen::GlobeRenderer> m_globeRenderer;
    
    // Store window pointer for callbacks
    GLFWwindow* m_window;
    
    // Static callback handling
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static std::unordered_map<GLFWwindow*, WorldGenScreen*> s_instances;
};