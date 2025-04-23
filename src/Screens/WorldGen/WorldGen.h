#pragma once

#include "../Screen.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include <unordered_map>
#include "../../Rendering/Layer.h"
#include "../../Rendering/Shapes/Rectangle.h"
#include "../../Rendering/Shapes/Text.h"
#include "../MainMenu/MainMenu.h" // For MenuButton structure
#include "TerrainGenerator.h"
#include "Planet/GlobeRenderer.h"

class WorldGenScreen : public Screen {
public:
    // Update constructor to accept Camera* and GLFWwindow*
    WorldGenScreen(Camera* camera, GLFWwindow* window);
    ~WorldGenScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;

private:
    void layoutUI();
    void createButton(const std::string& text, const glm::vec4& color, const glm::vec4& hoverColor, const std::function<void()>& callback);
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);
    void handleScroll(double xoffset, double yoffset);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
    // UI elements
    std::vector<MenuButton> buttons;
    float sidebarWidth;
    float lastCursorX;
    float lastCursorY;
    
    // World generation parameters
    int worldWidth;
    int worldHeight;
    float waterLevel;
    int seed;
    bool worldGenerated;
    std::unordered_map<WorldGen::TileCoord, WorldGen::TerrainData> generatedTerrainData; // Store generated terrain using TileCoord
    
    // Planet rendering
    std::unique_ptr<WorldGen::GlobeRenderer> m_globeRenderer;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_projectionMatrix;
    float m_cameraDistance;
    float m_rotationAngle;
    bool m_isDragging;
    
    // Layers
    std::shared_ptr<Rendering::Layer> starLayer;
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> previewLayer;
    std::shared_ptr<Rendering::Layer> controlsLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
    std::shared_ptr<Rendering::Layer> sidebarLayer;
};