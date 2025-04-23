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
#include "Plate/PlateGenerator.h"
#include "Plate/PlateRenderer.h"
#include "WorldGenUI.h"

struct GLFWwindow;
class Camera;

namespace WorldGen {
    class PlateGenerator;
    class PlateRenderer;
    class GlobeRenderer;
    struct TectonicPlate;
}

class WorldGenScreen : public Screen {
public:
    WorldGenScreen(Camera* camera, GLFWwindow* window);
    ~WorldGenScreen() override;

    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void handleInput() override;
    void onResize(int width, int height) override;

private:
    // Window callbacks
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void handleScroll(double xoffset, double yoffset);

    // Helper methods
    void renderStars(int width, int height);
    bool isPointInRect(float px, float py, float rx, float ry, float rw, float rh);

    // Used for managing multiple instances with callbacks
    static std::unordered_map<GLFWwindow*, WorldGenScreen*> s_instances;
    GLFWwindow* m_window;

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

    // Star layer (part of Planet display)
    std::shared_ptr<Rendering::Layer> starLayer;

    // UI system
    std::unique_ptr<WorldGen::WorldGenUI> m_worldGenUI;

    // Plate generation
    std::unique_ptr<WorldGen::PlateGenerator> m_plateGenerator;
    std::unique_ptr<WorldGen::PlateRenderer> m_plateRenderer;
    std::vector<std::shared_ptr<WorldGen::TectonicPlate>> m_plates;
    bool m_platesGenerated;

    // Input tracking
    float lastCursorX;
    float lastCursorY;
};