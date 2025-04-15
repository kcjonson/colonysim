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

class WorldGenScreen : public Screen {
public:
    WorldGenScreen();
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
    std::unordered_map<std::pair<int, int>, WorldGen::TerrainData> generatedTerrainData; // Store generated terrain
    
    // UI Layers
    std::shared_ptr<Rendering::Layer> backgroundLayer;
    std::shared_ptr<Rendering::Layer> controlsLayer;
    std::shared_ptr<Rendering::Layer> buttonLayer;
    std::shared_ptr<Rendering::Layer> previewLayer;
};