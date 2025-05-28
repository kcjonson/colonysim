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
    
    // COORDINATE SYSTEM FIX: Multi-chunk rendering coordinate system
    // 
    // PROBLEM: Camera was positioned at huge world coordinates (~10 million meters)
    // but tiles were positioned in small local coordinates (-83 to +83), causing
    // a coordinate system mismatch where tiles were invisible.
    //
    // SOLUTION: Use a local coordinate system for rendering:
    // - Camera positioned at (0,0,0) 
    // - All tiles positioned relative to current chunk center
    // - playerPosition tracks actual world coordinates for chunk management
    // - Camera movements are in local coordinates relative to current chunk
    // - When moving between chunks, tiles from multiple chunks are positioned
    //   correctly relative to each other using world coordinate differences
    if (camera) {
        camera->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        std::cout << "Camera positioned at origin (0,0,0) for local tile coordinate system" << std::endl;
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
    
    // TILE SYSTEM: Create initial tiles to fill window + preload radius
    auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();
    
    // Position camera at origin to start
    if (camera) {
        camera->setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
        camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
        std::cout << "Camera positioned at (0,0,5) looking at (0,0,0)" << std::endl;
    }
    
    if (chunks.count(currentChunk) > 0) {
        const auto& chunkData = chunks[currentChunk];
        std::cout << "Chunk loaded with " << chunkData->tiles.size() << " tile data entries" << std::endl;
        
        // Create initial tiles that fill the window plus preload radius
        createInitialTiles();
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
    
    // COORDINATE FIX: Camera position is now in local coordinates (relative to current chunk)
    // Convert camera position back to world coordinates for chunk management
    glm::vec3 cameraPos = camera->getPosition();
    glm::vec2 cameraLocalPos(cameraPos.x, cameraPos.y);
    
    // Add camera offset to player's base world position to get current world position
    glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    playerPosition = currentChunkWorld + cameraLocalPos;
    
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
                
                // COORDINATE FIX: Position tiles relative to current chunk center
                // Calculate offset from each chunk's center and position relative to current chunk
                glm::vec2 worldPos = tileToWorld(coord, localCoord);
                glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
                glm::vec2 tilePos = worldPos - currentChunkWorld;
                
                // Convert to pixel coordinates for storage (consistent with createInitialTiles)
                const float tilesPerMeter = config.getTilesPerMeter();
                glm::vec2 tilePosPixels = tilePos * tilesPerMeter;
                
                // Snap to tile grid (tiles are spaced by tileSize pixels)
                int pixelX = static_cast<int>(std::round(tilePosPixels.x / tileSize)) * static_cast<int>(tileSize);
                int pixelY = static_cast<int>(std::round(tilePosPixels.y / tileSize)) * static_cast<int>(tileSize);
                
                WorldGen::TileCoord pixelCoord{pixelX, pixelY};
                
                // Check if tile already exists
                if (tiles.count(pixelCoord) > 0) {
                    continue; // Skip duplicate
                }
                
                // Create tile at pixel position
                glm::vec2 tilePosForRendering(pixelX, pixelY);
                auto tile = std::make_shared<Rendering::Tile>(
                    tilePosForRendering, terrainData.height, terrainData.resource, 
                    terrainData.type
                );
                
                tiles[pixelCoord] = tile;
                worldLayer->addItem(tile);
                tile->setVisible(false); // Will be made visible by updateTileVisibility
                
                tilesCreated++;
            }
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
    }
    
    // Get the range of tiles that should be visible
    auto [minX, maxX, minY, maxY] = getVisibleTileRange(overscan);
    
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
    
    // Find tiles that should be visible (space by tileSize, not every pixel!)
    
    int tilesFound = 0;
    for (int y = minY; y <= maxY; y += static_cast<int>(tileSize)) {
        for (int x = minX; x <= maxX; x += static_cast<int>(tileSize)) {
            WorldGen::TileCoord coord{x, y};
            newVisibleTiles.insert(coord);
            if (tiles.count(coord) > 0) {
                tilesFound++;
            }
        }
    }
    
    if (!debugPrinted) {
        int totalPositions = ((maxX-minX)/static_cast<int>(tileSize) + 1) * ((maxY-minY)/static_cast<int>(tileSize) + 1);
        std::cout << "Found " << tilesFound << " visible tiles out of " << totalPositions << " checked positions" << std::endl;
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
    
    // Create and show newly visible tiles (on-demand creation)
    int newTilesCreated = 0;
    int tilesNotFound = 0;
    int tilesAlreadyExist = 0;
    
    for (const auto& coord : newVisibleTiles) {
        if (visibleTiles.count(coord) == 0) {
            auto it = tiles.find(coord);
            if (it == tiles.end()) {
                // Tile doesn't exist - create it on-demand
                if (createTileFromData(coord)) {
                    newTilesCreated++;
                } else {
                    tilesNotFound++;
                    if (tilesNotFound <= 5) {
                        std::cout << "Failed to create tile at (" << coord.x << ", " << coord.y << ")" << std::endl;
                    }
                }
            } else if (it->second) {
                // Tile exists - just show it
                it->second->setVisible(true);
                tilesAlreadyExist++;
            }
        }
    }
    
    if (newTilesCreated > 0 || tilesNotFound > 0) {
        std::cout << "Visibility update: created=" << newTilesCreated 
                  << ", not_found=" << tilesNotFound 
                  << ", already_exist=" << tilesAlreadyExist 
                  << ", chunks_loaded=" << chunks.size() << std::endl;
                  
        // Debug: show some of the tiles that weren't found
        if (tilesNotFound > 0) {
            int debugShown = 0;
            std::cout << "Sample of tiles not found:" << std::endl;
            for (const auto& coord : newVisibleTiles) {
                if (debugShown >= 5) break;
                if (visibleTiles.count(coord) == 0 && tiles.count(coord) == 0) {
                    std::cout << "  Missing tile at (" << coord.x << ", " << coord.y << ")" << std::endl;
                    debugShown++;
                }
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
    
    // COORDINATE FIX: Return bounds in pixel coordinates to match tile storage
    // Camera and tiles are both in local coordinates relative to chunk center
    glm::vec3 cameraPos = camera->getPosition();
    
    // Return pixel bounds directly since tiles are stored in pixel coordinates
    return glm::vec4(
        cameraPos.x + camera->getProjectionLeft(),
        cameraPos.x + camera->getProjectionRight(),
        cameraPos.y + camera->getProjectionBottom(),
        cameraPos.y + camera->getProjectionTop()
    );
}

/**
 * Calculate the range of tile coordinates that should be visible on screen.
 * @param overscan Additional tiles to include beyond the visible area (for preloading)
 * @return Tuple of (minX, maxX, minY, maxY) in pixel coordinates
 */
std::tuple<int, int, int, int> World::getVisibleTileRange(int overscan) const {
    if (!camera) return {0, 0, 0, 0};
    
    auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();
    
    glm::vec4 bounds = getCameraBounds();
    
    // COORDINATE FIX: bounds are now in pixel coordinates, snap to tile grid
    // Tiles are spaced by tileSize pixels, so snap bounds to tileSize grid
    // overscan is in tile units, so multiply by tileSize to get pixels
    int overscanPixels = overscan * static_cast<int>(tileSize);
    int minX = static_cast<int>(std::floor(bounds.x / tileSize)) * static_cast<int>(tileSize) - overscanPixels;
    int maxX = static_cast<int>(std::ceil(bounds.y / tileSize)) * static_cast<int>(tileSize) + overscanPixels;
    int minY = static_cast<int>(std::floor(bounds.z / tileSize)) * static_cast<int>(tileSize) - overscanPixels;
    int maxY = static_cast<int>(std::ceil(bounds.w / tileSize)) * static_cast<int>(tileSize) + overscanPixels;
    
    return {minX, maxX, minY, maxY};
}

/**
 * Create the initial set of tiles needed to fill the window plus preload radius.
 * This ensures tiles are ready for rendering before the first frame.
 */
void World::createInitialTiles() {
    // Use the same logic as updateTileVisibility to determine which tiles should exist initially
    auto& config = ConfigManager::getInstance();
    const int overscan = config.getTileCullingOverscan();
    
    auto [minX, maxX, minY, maxY] = getVisibleTileRange(overscan);
    
    std::cout << "Creating initial tiles for range: X[" << minX << ", " << maxX << "] Y[" << minY << ", " << maxY << "]" << std::endl;
    
    // Create tiles for the visible area, but space them by tileSize (not every pixel!)
    const float tileSize = config.getTileSize();
    
    int tilesCreated = 0;
    for (int y = minY; y <= maxY; y += static_cast<int>(tileSize)) {
        for (int x = minX; x <= maxX; x += static_cast<int>(tileSize)) {
            WorldGen::TileCoord coord{x, y};
            if (createTileFromData(coord)) {
                tilesCreated++;
            }
        }
    }
    
    std::cout << "Created " << tilesCreated << " initial tiles" << std::endl;
}

/**
 * Create a tile instance from terrain data for the given coordinate.
 * Searches through loaded chunks to find terrain data and creates a renderable tile.
 * @param coord Tile coordinate in pixel space
 * @return true if tile was successfully created, false if no terrain data found
 */
bool World::createTileFromData(const WorldGen::TileCoord& coord) {
    // TODO: Optimize chunk lookup for performance with many chunks
    // Instead of searching all chunks, calculate which chunk should contain this coordinate
    // and only look in that chunk + immediate neighbors
    
    static int debugCount = 0;
    bool showDebug = debugCount < 20;
    if (showDebug) {
        std::cout << "createTileFromData: Looking for tile at pixel coord (" << coord.x << ", " << coord.y << ")" << std::endl;
        debugCount++;
    }
    
    // Find terrain data for this coordinate from loaded chunks
    for (const auto& [chunkCoord, chunkData] : chunks) {
        if (!chunkData) continue;
        
        // Convert pixel coordinate back to local chunk coordinate
        auto& config = ConfigManager::getInstance();
        const int chunkSize = config.getChunkSize();
        const float tileSize = config.getTileSize();
        
        // Calculate this chunk's offset from the current chunk
        glm::vec2 thisChunkWorld = sphereToWorld(chunkCoord.centerOnSphere);
        glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
        glm::vec2 chunkOffset = thisChunkWorld - currentChunkWorld;
        
        // Convert chunk offset from meters to pixels
        const float tilesPerMeter = config.getTilesPerMeter();
        glm::vec2 chunkOffsetPixels = chunkOffset * tilesPerMeter;
        
        // Convert pixel coordinate to local coordinate within this chunk
        int localX = static_cast<int>((coord.x - chunkOffsetPixels.x) / tileSize + chunkSize * 0.5f);
        int localY = static_cast<int>((coord.y - chunkOffsetPixels.y) / tileSize + chunkSize * 0.5f);
        
        // Check if this coordinate is within this chunk
        if (localX >= 0 && localX < chunkSize && localY >= 0 && localY < chunkSize) {
            WorldGen::TileCoord localCoord{localX, localY};
            auto terrainIt = chunkData->tiles.find(localCoord);
            
            if (showDebug) {
                std::cout << "  Checking chunk at offset (" << chunkOffset.x << ", " << chunkOffset.y 
                         << "), pixel offset (" << chunkOffsetPixels.x << ", " << chunkOffsetPixels.y
                         << "), local coord (" << localX << ", " << localY << ")"
                         << " - Found: " << (terrainIt != chunkData->tiles.end() ? "YES" : "NO") << std::endl;
            }
            
            if (terrainIt != chunkData->tiles.end()) {
                // Found terrain data - create the tile
                const auto& terrainData = terrainIt->second;
                glm::vec2 tilePos(coord.x, coord.y);  // Position in pixel coordinates
                
                auto tile = std::make_shared<Rendering::Tile>(
                    tilePos, terrainData.height, terrainData.resource, terrainData.type
                );
                
                tiles[coord] = tile;
                worldLayer->addItem(tile);
                tile->setVisible(true);
                
                if (showDebug) {
                    std::cout << "  SUCCESS: Created and stored tile at pixel coord (" << coord.x << ", " << coord.y << ")" << std::endl;
                }
                
                return true;  // Successfully created
            }
        }
    }
    
    return false;  // No terrain data found
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
    gameState.set("world.shownTiles", std::to_string(visibleTileCount));
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