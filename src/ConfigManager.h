#pragma once

#include <string>

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadConfig(const std::string& filepath);
    
    // Window settings
    int getWindowWidth() const { return windowWidth; }
    int getWindowHeight() const { return windowHeight; }
    const std::string& getWindowTitle() const { return windowTitle; }
    
    // Camera settings
    float getViewHeight() const { return viewHeight; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }
    
    // World settings
    int getChunkSize() const { return chunkSize; }
    float getTileSize() const { return tileSize; }
    float getTilesPerMeter() const { return tilesPerMeter; }
    int getPreloadRadius() const { return preloadRadius; }
    int getUnloadRadius() const { return unloadRadius; }
    int getMaxLoadedChunks() const { return maxLoadedChunks; }
    int getMaxNewTilesPerFrame() const { return maxNewTilesPerFrame; }
    int getTileCullingOverscan() const { return tileCullingOverscan; }
    int getTileSampleRate() const { return tileSampleRate; }
    int getChunkEdgeTriggerDistance() const { return chunkEdgeTriggerDistance; }
    int getNumChunksToKeep() const { return numChunksToKeep; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    bool configLoaded = false;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Window settings
    int windowWidth = 800;
    int windowHeight = 600;
    std::string windowTitle = "Colony Sim";

    // Camera settings
    float viewHeight = 1000.0f;
    float nearPlane = -1000.0f;
    float farPlane = 1000.0f;
    
    // World settings
    int chunkSize = 1000;
    float tileSize = 20.0f;
    float tilesPerMeter = 1.0f;
    int preloadRadius = 1;
    int unloadRadius = 2;
    int maxLoadedChunks = 9;
    int numChunksToKeep = 25;
    int maxNewTilesPerFrame = 100;
    int tileCullingOverscan = 3;
    int tileSampleRate = 4;
    int chunkEdgeTriggerDistance = 10;
}; 