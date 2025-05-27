#include "World.h"
#include "../../ConfigManager.h"
#include "../WorldGen/Core/Util.h"
#include "../WorldGen/Core/ChunkGenerator.h"
#include <iostream>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

World::World(GameState& gameState, 
                         const std::string& seed, 
                         Camera* camera, 
                         GLFWwindow* window,
                         const WorldGen::Generators::World* sphericalWorld,
                         std::unique_ptr<WorldGen::Core::ChunkData> initialChunk,
                         const glm::vec3& landingLocation)
    : gameState(gameState),
      seed(seed),
      camera(camera),
      sphericalWorld(sphericalWorld),
      landingLocation(landingLocation),
      worldLayer(std::make_shared<Rendering::Layer>(50.0f, Rendering::ProjectionType::WorldSpace, camera, window))
{
    std::cout << "Initializing World at landing location: " 
              << landingLocation.x << ", " << landingLocation.y << ", " << landingLocation.z << std::endl;
    
    // Calculate player's starting position in global world coordinates
    // Convert from landing location on sphere to world coordinates
    playerPosition = sphereToWorld(landingLocation);
    
    std::cout << "Player starting at world position: (" 
              << playerPosition.x << ", " << playerPosition.y << ") meters from origin" << std::endl;
    
    // Store the initial chunk
    // The initial chunk is centered at the landing location on the sphere
    if (initialChunk) {
        currentChunk = initialChunk->coord;
        chunks[currentChunk] = std::move(initialChunk);
    }
    
    // Position camera at player location
    if (camera) {
        camera->setPosition(glm::vec3(playerPosition.x, playerPosition.y, 0.0f));
    }
}

World::~World() {
    // Stop the background thread
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        running = false;
    }
    chunkCondVar.notify_all();
    
    if (chunkGeneratorThread.joinable()) {
        chunkGeneratorThread.join();
    }
}

bool World::initialize() {
    // Start the background chunk generation thread
    chunkGeneratorThread = std::thread(&World::chunkGeneratorThreadFunc, this);
    
    // Initialize tiles from the initial chunk
    auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();
    
    if (chunks.count(currentChunk) > 0) {
        const auto& chunkData = chunks[currentChunk];
        std::cout << "Initializing " << chunkData->tiles.size() << " tiles from initial chunk" << std::endl;
        int tileCount = 0;
        for (const auto& [localCoord, terrainData] : chunkData->tiles) {
            // Calculate tile position directly relative to player (center chunk at player position)
            auto& config = ConfigManager::getInstance();
            const int chunkSize = config.getChunkSize();
            const float tilesPerMeter = config.getTilesPerMeter();
            
            // Calculate tile offset from chunk center (localCoord ranges 0 to chunkSize-1)
            float offsetX = (localCoord.x - chunkSize * 0.5f) / tilesPerMeter;
            float offsetY = (localCoord.y - chunkSize * 0.5f) / tilesPerMeter;
            
            glm::vec2 relativePos(offsetX, offsetY);
            glm::vec2 tilePos(relativePos.x * tileSize, relativePos.y * tileSize);
            
            tileCount++;
            
            // Create tile with forced bright colors for testing
            glm::vec4 testColor = terrainData.color;
            if (tileCount < 25) {
                // Force first 25 tiles to be bright colors for visibility testing
                testColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // Bright red
                if (tileCount % 2 == 0) {
                    testColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // Bright green
                }
            }
            
            auto tile = std::make_shared<Rendering::Tile>(
                tilePos, terrainData.height, terrainData.resource, 
                terrainData.type, testColor
            );
            
            // Store tiles using meter coordinates (centered around 0,0)
            WorldGen::TileCoord relativeCoord{
                static_cast<int>(relativePos.x),
                static_cast<int>(relativePos.y)
            };
            
            // TEMPORARY FIX: Force first few tiles to be at visible coordinates for testing
            if (tileCount < 25) {
                relativeCoord.x = (tileCount % 5) - 2;  // -2, -1, 0, 1, 2
                relativeCoord.y = (tileCount / 5) - 2;  // -2, -1, 0, 1, 2
                // Recalculate tile position for these forced coordinates
                tilePos = glm::vec2(relativeCoord.x * tileSize, relativeCoord.y * tileSize);
            }
            
            if (tileCount < 5) {
                std::cout << "  tile offset (" << offsetX << "," << offsetY << ") -> relative (" << relativePos.x << "," << relativePos.y << ")" << std::endl;
                std::cout << "  -> relative coord (" << relativeCoord.x << "," << relativeCoord.y << ")" << std::endl;
                std::cout << "  color: (" << terrainData.color.r << "," << terrainData.color.g << "," << terrainData.color.b << "," << terrainData.color.a << ")" << std::endl;
            }
            
            tiles[relativeCoord] = tile;
            worldLayer->addItem(tile);
            
            // BYPASS VISIBILITY SYSTEM - Force all tiles visible for testing
            tile->setVisible(true);
            
            if (tileCount < 5) {
                std::cout << "  -> Tile added to worldLayer and set VISIBLE" << std::endl;
            }
            
            if (tileCount < 5) {
                std::cout << "Tile " << tileCount << " at relative (" << relativeCoord.x << "," << relativeCoord.y 
                          << ") screen (" << tilePos.x << "," << tilePos.y << ")" << std::endl;
            }
        }
        std::cout << "Total tiles added to worldLayer: " << tileCount << std::endl;
    } else {
        std::cout << "ERROR: No initial chunk found in chunks map!" << std::endl;
    }
    
    // Load adjacent chunks
    loadAdjacentChunks();
    
    return true;
}


void World::update(float deltaTime) {
    auto& config = ConfigManager::getInstance();
    
    // Update current chunk based on camera position
    updateCurrentChunk();
    
    // Integrate any chunks that finished loading
    integrateLoadedChunks();
    
    // Update tile visibility
    static bool firstUpdate = true;
    if (cameraViewChanged() || firstUpdate) {
        if (firstUpdate) {
            std::cout << "First update - forcing tile visibility update" << std::endl;
            firstUpdate = false;
        }
        updateTileVisibility();
    }
    
    // Memory logging
    timeSinceLastLog += deltaTime;
    if (timeSinceLastLog >= 0.5f) {
        logMemoryUsage();
        timeSinceLastLog = 0.0f;
    }
}

void World::render() {
    if (!worldLayer) {
        std::cerr << "ERROR: Attempting to render with null worldLayer" << std::endl;
        return;
    }
    
    worldLayer->render(false);
}

WorldGen::Core::ChunkCoord World::worldToChunk(const glm::vec2& worldPos) const {
    /**
     * COORDINATE CONVERSION: World to Chunk
     * 
     * This method determines which chunk contains a given world position.
     * Since chunks are indexed by their center position on the sphere,
     * we need to:
     * 1. Convert world position to sphere position
     * 2. Determine which chunk center is closest
     * 
     * For now, we'll generate chunks on a regular grid and use the
     * nearest grid point as the chunk center.
     */
    
    auto& config = ConfigManager::getInstance();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    const float chunkSizeMeters = chunkSize / tilesPerMeter;
    
    // Determine grid indices for this position
    int gridX = static_cast<int>(std::round(worldPos.x / chunkSizeMeters));
    int gridY = static_cast<int>(std::round(worldPos.y / chunkSizeMeters));
    
    // Calculate the center of this grid cell in world coordinates
    glm::vec2 chunkCenterWorld(gridX * chunkSizeMeters, gridY * chunkSizeMeters);
    
    // Convert to sphere coordinates
    glm::vec3 chunkCenterSphere = worldToSphere(chunkCenterWorld);
    
    return WorldGen::Core::ChunkCoord(chunkCenterSphere);
}

WorldGen::TileCoord World::worldToLocalTile(const glm::vec2& worldPos) const {
    auto& config = ConfigManager::getInstance();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    
    float tileX = worldPos.x * tilesPerMeter;
    float tileY = worldPos.y * tilesPerMeter;
    
    // Calculate local tile within chunk
    int localX = static_cast<int>(std::floor(tileX)) % chunkSize;
    int localY = static_cast<int>(std::floor(tileY)) % chunkSize;
    
    // Handle negative coordinates
    if (localX < 0) localX += chunkSize;
    if (localY < 0) localY += chunkSize;
    
    return WorldGen::TileCoord{localX, localY};
}

glm::vec2 World::tileToWorld(const WorldGen::Core::ChunkCoord& chunk, 
                                   const WorldGen::TileCoord& localTile) const {
    /**
     * COORDINATE CONVERSION: Chunk + Tile to World
     * 
     * Given a chunk (identified by its sphere center) and a local tile,
     * calculate the world position of that tile.
     * 
     * This requires us to:
     * 1. Convert chunk center from sphere to world coordinates
     * 2. Calculate tile offset from chunk center
     * 3. Add offset to get final world position
     */
    
    auto& config = ConfigManager::getInstance();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    
    // Get chunk center in world coordinates
    glm::vec2 chunkCenterWorld = sphereToWorld(chunk.centerOnSphere);
    
    // Calculate tile offset from chunk center
    // Tiles are arranged with (0,0) at bottom-left, so center is at (chunkSize/2, chunkSize/2)
    float offsetX = (localTile.x - chunkSize * 0.5f) / tilesPerMeter;
    float offsetY = (localTile.y - chunkSize * 0.5f) / tilesPerMeter;
    
    return chunkCenterWorld + glm::vec2(offsetX, offsetY);
}

void World::updateCurrentChunk() {
    if (!camera) return;
    
    // Update player position based on camera
    glm::vec3 cameraPos = camera->getPosition();
    playerPosition = glm::vec2(cameraPos.x, cameraPos.y);
    
    // Determine which chunk the player is in
    WorldGen::Core::ChunkCoord newChunk = worldToChunk(playerPosition);
    
    // Check if we've moved to a different chunk
    if (!(newChunk == currentChunk)) {
        glm::vec2 oldChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
        glm::vec2 newChunkWorld = sphereToWorld(newChunk.centerOnSphere);
        
        std::cout << "Moving from chunk at world pos (" << oldChunkWorld.x << ", " << oldChunkWorld.y 
                  << ") to chunk at world pos (" << newChunkWorld.x << ", " << newChunkWorld.y << ")" << std::endl;
        
        currentChunk = newChunk;
        
        // Trigger loading of new adjacent chunks and unloading of distant ones
        loadAdjacentChunks();
        unloadDistantChunks();
    }
}

void World::loadAdjacentChunks() {
    /**
     * Load chunks adjacent to the current chunk.
     * 
     * Since chunks are indexed by sphere position, we need to calculate
     * the neighboring chunk centers on the sphere.
     */
    
    auto& config = ConfigManager::getInstance();
    const int preloadRadius = config.getPreloadRadius();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    const float chunkSizeMeters = chunkSize / tilesPerMeter;
    
    // Get current chunk's world position
    glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    
    // Generate chunks in a grid pattern around current chunk
    for (int dy = -preloadRadius; dy <= preloadRadius; dy++) {
        for (int dx = -preloadRadius; dx <= preloadRadius; dx++) {
            // Calculate neighbor chunk center in world coordinates
            glm::vec2 neighborWorld = currentChunkWorld + glm::vec2(dx * chunkSizeMeters, dy * chunkSizeMeters);
            
            // Convert to sphere coordinates
            glm::vec3 neighborSphere = worldToSphere(neighborWorld);
            WorldGen::Core::ChunkCoord neighborCoord(neighborSphere);
            
            // Check if chunk is already loaded or being generated
            bool needsLoading = false;
            {
                std::lock_guard<std::mutex> lock(chunkMutex);
                needsLoading = chunks.count(neighborCoord) == 0 && 
                              pendingChunks.count(neighborCoord) == 0;
            }
            
            if (needsLoading) {
                generateChunkAsync(neighborCoord);
            }
        }
    }
}

void World::unloadDistantChunks() {
    /**
     * Unload chunks that are too far from the player.
     * 
     * Distance is measured in world coordinates, not chunk grid indices.
     */
    
    auto& config = ConfigManager::getInstance();
    const int unloadRadius = config.getUnloadRadius();
    const float tileSize = config.getTileSize();
    const int chunkSize = config.getChunkSize();
    const float tilesPerMeter = config.getTilesPerMeter();
    const float chunkSizeMeters = chunkSize / tilesPerMeter;
    const float unloadDistanceMeters = unloadRadius * chunkSizeMeters;
    
    // Get current chunk's world position
    glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    
    std::vector<WorldGen::Core::ChunkCoord> toUnload;
    
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        for (const auto& [coord, chunk] : chunks) {
            // Calculate distance between chunks in world coordinates
            glm::vec2 chunkWorld = sphereToWorld(coord.centerOnSphere);
            float distance = glm::distance(chunkWorld, currentChunkWorld);
            
            if (distance > unloadDistanceMeters) {
                toUnload.push_back(coord);
            }
        }
    }
    
    // Unload chunks and their tiles
    for (const auto& coord : toUnload) {
        glm::vec2 chunkWorld = sphereToWorld(coord.centerOnSphere);
        std::cout << "Unloading chunk at world pos: (" << chunkWorld.x << ", " << chunkWorld.y << ")" << std::endl;
        
        // Remove tiles from rendering
        // We need to find all tiles that belong to this chunk
        // Since we don't track which tiles belong to which chunk,
        // we'll need to iterate through all tiles and check their position
        std::vector<WorldGen::TileCoord> tilesToRemove;
        
        for (const auto& [tileCoord, tile] : tiles) {
            // Get the tile's world position
            glm::vec2 tileWorldPos(
                tileCoord.x / tilesPerMeter,
                tileCoord.y / tilesPerMeter
            );
            
            // Check if this tile belongs to the chunk being unloaded
            WorldGen::Core::ChunkCoord tileChunk = worldToChunk(tileWorldPos);
            if (tileChunk == coord) {
                tilesToRemove.push_back(tileCoord);
            }
        }
        
        // Remove the tiles
        for (const auto& tileCoord : tilesToRemove) {
            auto tileIt = tiles.find(tileCoord);
            if (tileIt != tiles.end()) {
                worldLayer->removeItem(tileIt->second);
                tiles.erase(tileIt);
            }
        }
        
        // Remove chunk
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            chunks.erase(coord);
        }
    }
}

void World::generateChunkAsync(const WorldGen::Core::ChunkCoord& coord) {
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        
        // Mark as pending
        auto pending = std::make_unique<WorldGen::Core::ChunkData>();
        pending->coord = coord;
        pending->isGenerating = true;
        pendingChunks[coord] = std::move(pending);
        
        // Add to queue
        chunkLoadQueue.push(coord);
    }
    
    chunkCondVar.notify_one();
}

void World::chunkGeneratorThreadFunc() {
    while (running) {
        WorldGen::Core::ChunkCoord coord;
        
        // Get next chunk to generate
        {
            std::unique_lock<std::mutex> lock(chunkMutex);
            chunkCondVar.wait(lock, [this] { 
                return !chunkLoadQueue.empty() || !running; 
            });
            
            if (!running) break;
            
            coord = chunkLoadQueue.front();
            chunkLoadQueue.pop();
        }
        
        // Generate the chunk
        generateChunk(coord);
    }
}

void World::generateChunk(const WorldGen::Core::ChunkCoord& coord) {
    /**
     * Generate a chunk using the ChunkGenerator.
     * 
     * This method is called by the background thread to generate
     * chunks asynchronously.
     */
    
    // Use ChunkGenerator to create the chunk
    auto chunk = WorldGen::Core::ChunkGenerator::generateChunk(*sphericalWorld, coord.centerOnSphere);
    
    if (!chunk) {
        std::cerr << "ERROR: Failed to generate chunk" << std::endl;
        return;
    }
    
    // Move to pending chunks for main thread to integrate
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        pendingChunks[coord] = std::move(chunk);
    }
}

void World::integrateLoadedChunks() {
    auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();
    const int chunkSize = config.getChunkSize();
    const int maxNewTilesPerFrame = config.getMaxNewTilesPerFrame();
    
    std::vector<WorldGen::Core::ChunkCoord> toIntegrate;
    
    // Find chunks ready to integrate
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        for (const auto& [coord, chunk] : pendingChunks) {
            if (chunk->isLoaded && !chunk->isGenerating) {
                toIntegrate.push_back(coord);
            }
        }
    }
    
    int tilesCreated = 0;
    
    // Integrate chunks
    for (const auto& coord : toIntegrate) {
        std::unique_ptr<WorldGen::Core::ChunkData> chunk;
        
        // Move chunk from pending to active
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            auto it = pendingChunks.find(coord);
            if (it != pendingChunks.end()) {
                chunk = std::move(it->second);
                pendingChunks.erase(it);
                chunks[coord] = std::move(chunk);
            }
        }
        
        // Create tiles for rendering
        if (chunks.count(coord) > 0) {
            const auto& chunkData = chunks[coord];
            
            for (const auto& [localCoord, terrainData] : chunkData->tiles) {
                if (tilesCreated >= maxNewTilesPerFrame) {
                    break; // Limit tiles per frame
                }
                
                // Calculate global position
                glm::vec2 worldPos = tileToWorld(coord, localCoord);
                // Make tile position relative to player starting position
                glm::vec2 relativePos = worldPos - playerPosition;
                glm::vec2 tilePos(relativePos.x * tileSize, relativePos.y * tileSize);
                
                // Create tile
                auto tile = std::make_shared<Rendering::Tile>(
                    tilePos, terrainData.height, terrainData.resource, 
                    terrainData.type, terrainData.color
                );
                
                // Store tiles using meter coordinates
                WorldGen::TileCoord relativeCoord{
                    static_cast<int>(relativePos.x),
                    static_cast<int>(relativePos.y)
                };
                
                tiles[relativeCoord] = tile;
                worldLayer->addItem(tile);
                tile->setVisible(false); // Will be made visible by updateTileVisibility
                
                tilesCreated++;
            }
        }
        
        if (tilesCreated >= maxNewTilesPerFrame) {
            break; // Process remaining chunks next frame
        }
    }
    
    if (tilesCreated > 0) {
        std::cout << "Integrated " << tilesCreated << " tiles from new chunks" << std::endl;
    }
}

void World::updateTileVisibility() {
    auto& config = ConfigManager::getInstance();
    const int overscan = config.getTileCullingOverscan();
    const float tileSize = config.getTileSize();
    
    glm::vec4 bounds = getCameraBounds();
    
    // Debug output first time
    static bool debugPrinted = false;
    if (!debugPrinted) {
        glm::vec3 camPos = camera->getPosition();
        std::cout << "\n=== TILE VISIBILITY DEBUG ===" << std::endl;
        std::cout << "Camera position: (" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")" << std::endl;
        std::cout << "Camera bounds: [" << bounds.x << ", " << bounds.y << "] x [" << bounds.z << ", " << bounds.w << "]" << std::endl;
        std::cout << "Player position: (" << playerPosition.x << ", " << playerPosition.y << ")" << std::endl;
        std::cout << "Tile size: " << tileSize << std::endl;
        std::cout << "Total tiles available: " << tiles.size() << std::endl;
        debugPrinted = true;
    }
    
    // Calculate visible range in meter coordinates (not tile coordinates)
    int minX = static_cast<int>(std::floor(bounds.x)) - overscan;
    int maxX = static_cast<int>(std::ceil(bounds.y)) + overscan;
    int minY = static_cast<int>(std::floor(bounds.z)) - overscan;
    int maxY = static_cast<int>(std::ceil(bounds.w)) + overscan;
    
    // Debug output first time
    if (!debugPrinted) {
        std::cout << "Visible tile range: X[" << minX << ", " << maxX << "] Y[" << minY << ", " << maxY << "]" << std::endl;
        
        // Show first few actual tile coordinates
        std::cout << "First 10 tile coordinates in storage:" << std::endl;
        int count = 0;
        for (const auto& [coord, tile] : tiles) {
            if (count++ < 10) {
                std::cout << "  Tile at (" << coord.x << ", " << coord.y << ")" << std::endl;
            }
        }
        
        // Show what coordinates we're searching in
        std::cout << "Looking for tiles in range:" << std::endl;
        std::cout << "  X: " << minX << " to " << maxX << std::endl;
        std::cout << "  Y: " << minY << " to " << maxY << std::endl;
        std::cout << "=============================\n" << std::endl;
    }
    
    std::unordered_set<WorldGen::TileCoord> newVisibleTiles;
    
    // Find tiles that should be visible
    int tilesFound = 0;
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            WorldGen::TileCoord coord{x, y};
            if (tiles.count(coord) > 0) {
                newVisibleTiles.insert(coord);
                tilesFound++;
            }
        }
    }
    
    if (!debugPrinted) {
        std::cout << "Found " << tilesFound << " visible tiles out of " << ((maxX-minX+1) * (maxY-minY+1)) << " checked positions" << std::endl;
        if (tilesFound > 0) {
            std::cout << "Making " << tilesFound << " tiles visible!" << std::endl;
        }
    }
    
    // Hide tiles no longer visible
    for (const auto& coord : visibleTiles) {
        if (newVisibleTiles.count(coord) == 0) {
            auto it = tiles.find(coord);
            if (it != tiles.end() && it->second) {
                it->second->setVisible(false);
            }
        }
    }
    
    // Show newly visible tiles
    for (const auto& coord : newVisibleTiles) {
        if (visibleTiles.count(coord) == 0) {
            auto it = tiles.find(coord);
            if (it != tiles.end() && it->second) {
                it->second->setVisible(true);
            }
        }
    }
    
    visibleTiles = std::move(newVisibleTiles);
    
    if (!debugPrinted) {
        std::cout << "Setting " << visibleTiles.size() << " tiles as visible" << std::endl;
        debugPrinted = true;
    }
    
    // Update camera state
    lastCameraPos = camera->getPosition();
    lastCameraBounds = glm::vec4(
        camera->getProjectionLeft(),
        camera->getProjectionRight(),
        camera->getProjectionBottom(),
        camera->getProjectionTop()
    );
}

bool World::cameraViewChanged() const {
    if (!camera) return false;
    
    const float epsilon = 0.001f;
    
    glm::vec3 currentPos = camera->getPosition();
    glm::vec4 currentBounds(
        camera->getProjectionLeft(),
        camera->getProjectionRight(),
        camera->getProjectionBottom(),
        camera->getProjectionTop()
    );
    
    bool posChanged = glm::distance(currentPos, lastCameraPos) > epsilon;
    bool boundsChanged = 
        std::abs(currentBounds.x - lastCameraBounds.x) > epsilon ||
        std::abs(currentBounds.y - lastCameraBounds.y) > epsilon ||
        std::abs(currentBounds.z - lastCameraBounds.z) > epsilon ||
        std::abs(currentBounds.w - lastCameraBounds.w) > epsilon;
    
    return posChanged || boundsChanged;
}

glm::vec4 World::getCameraBounds() const {
    if (!camera) {
        return glm::vec4(-10.0f, 10.0f, -10.0f, 10.0f);
    }
    
    // Get camera bounds relative to player position (since tiles are stored relative to player)
    glm::vec3 cameraPos = camera->getPosition();
    glm::vec2 cameraRelativePos = glm::vec2(cameraPos.x, cameraPos.y) - playerPosition;
    
    return glm::vec4(
        cameraRelativePos.x + camera->getProjectionLeft(),
        cameraRelativePos.x + camera->getProjectionRight(),
        cameraRelativePos.y + camera->getProjectionBottom(),
        cameraRelativePos.y + camera->getProjectionTop()
    );
}

void World::logMemoryUsage() const {
    size_t totalChunks = chunks.size();
    size_t totalTiles = tiles.size();
    size_t visibleTileCount = visibleTiles.size();
    
    // Estimate memory usage
    float chunkMemoryKB = totalChunks * sizeof(WorldGen::Core::ChunkData) / 1024.0f;
    float tileMemoryKB = totalTiles * sizeof(Rendering::Tile) / 1024.0f;
    float totalMemoryKB = chunkMemoryKB + tileMemoryKB;
    
    gameState.set("world.chunks", std::to_string(totalChunks));
    gameState.set("world.totalTiles", std::to_string(totalTiles));
    gameState.set("world.visibleTiles", std::to_string(visibleTileCount));
    gameState.set("world.chunkMemKB", std::to_string(static_cast<int>(chunkMemoryKB)) + " KB");
    gameState.set("world.tileMemKB", std::to_string(static_cast<int>(tileMemoryKB)) + " KB");
    gameState.set("world.totalMemKB", std::to_string(static_cast<int>(totalMemoryKB)) + " KB");
}

glm::vec3 World::worldToSphere(const glm::vec2& worldPos) const {
    /**
     * COORDINATE CONVERSION: World to Sphere
     * 
     * We use a cylindrical equal-area projection where:
     * - X distance represents movement along latitude lines (east/west)
     * - Y distance represents movement along meridians (north/south)
     * 
     * The world origin (0,0) maps to sphere point (1,0,0) - equator at prime meridian
     */
    
    // Convert distances to angles
    float theta = worldPos.x / PLANET_RADIUS;  // Longitude in radians
    float phi = worldPos.y / PLANET_RADIUS;    // Latitude in radians
    
    // Clamp latitude to valid range [-π/2, π/2]
    const float halfPi = static_cast<float>(M_PI) * 0.5f;
    phi = glm::clamp(phi, -halfPi, halfPi);
    
    // Convert to Cartesian coordinates on unit sphere
    float cosLat = std::cos(phi);
    return glm::vec3(
        std::cos(theta) * cosLat,  // X: east/west
        std::sin(phi),             // Y: up/down (toward poles)
        std::sin(theta) * cosLat   // Z: north/south
    );
}

glm::vec2 World::sphereToWorld(const glm::vec3& spherePos) const {
    /**
     * COORDINATE CONVERSION: Sphere to World
     * 
     * Inverse of worldToSphere conversion.
     * Maps 3D sphere positions to 2D world coordinates.
     */
    
    // Normalize to ensure we're on the unit sphere
    glm::vec3 pos = glm::normalize(spherePos);
    
    // Extract spherical coordinates
    float theta = std::atan2(pos.z, pos.x);  // Longitude
    float phi = std::asin(glm::clamp(pos.y, -1.0f, 1.0f));  // Latitude
    
    // Convert to world distances
    return glm::vec2(
        theta * PLANET_RADIUS,  // X: east/west distance
        phi * PLANET_RADIUS     // Y: north/south distance
    );
}