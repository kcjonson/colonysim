#include "World.h"
#include "../../ConfigManager.h"
#include "../WorldGen/Core/Util.h"
#include "../WorldGen/Core/ChunkGenerator.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>

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
        touchChunk(currentChunk);  // Add to LRU cache
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
    
    // Don't preload adjacent chunks at startup - wait for edge detection
    // This prevents loading unnecessary chunks when the game starts
    
    return true;
}


void World::update(float deltaTime) {
    auto& config = ConfigManager::getInstance();
    
    // Update current chunk based on camera position
    updateCurrentChunk();
    
    // Update player position for edge detection
    // Since we're not switching chunks, we need to track player position manually
    if (camera) {
        glm::vec3 cameraPos = camera->getPosition();
        glm::vec2 cameraLocalPos(cameraPos.x, cameraPos.y);
        
        // Player position is camera position relative to the fixed current chunk
        glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
        playerPosition = currentChunkWorld + cameraLocalPos;
    }
    
    // Integrate any chunks that finished loading
    integrateLoadedChunks();
    
    // Update tile visibility
    static bool firstUpdate = true;
    if (cameraViewChanged() || firstUpdate) {
        firstUpdate = false;
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
    // DISABLED: Don't automatically switch current chunk
    // The current chunk defines our coordinate system origin (0,0).
    // Switching it would invalidate all tile positions since they're relative to current chunk.
    // All tiles are positioned relative to the initial currentChunk set in initialize().
    // The edge detection in checkAndLoadNearbyChunks() will handle loading adjacent chunks.
    return;
    
    // Original logic kept for reference:
    // if (!camera) return;
    // 
    // // COORDINATE FIX: Camera position is now in local coordinates (relative to current chunk)
    // // Convert camera position back to world coordinates for chunk management
    // glm::vec3 cameraPos = camera->getPosition();
    // glm::vec2 cameraLocalPos(cameraPos.x, cameraPos.y);
    // 
    // // Add camera offset to player's base world position to get current world position
    // glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    // playerPosition = currentChunkWorld + cameraLocalPos;
    // 
    // // Determine which chunk the player is in
    // WorldGen::Core::ChunkCoord newChunk = worldToChunk(playerPosition);
    // 
    // // Check if we've moved to a different chunk
    // if (!(newChunk == currentChunk)) {
    //     glm::vec2 oldChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    //     glm::vec2 newChunkWorld = sphereToWorld(newChunk.centerOnSphere);
    //     
    //     
    //     currentChunk = newChunk;
    //     touchChunk(currentChunk);  // Update LRU for current chunk
    //     
    //     // Don't load all adjacent chunks when switching - let edge detection handle it
    //     // loadAdjacentChunks();  // This was loading too many chunks
    //     // unloadDistantChunks();  // Replaced by LRU system
    // }
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
                
                // Also remove from tracking maps
                tileToChunkMap.erase(tileCoord);
                visibleTiles.erase(tileCoord);
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
        bool wasAdded = false;
        
        // Move chunk from pending to active
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            auto it = pendingChunks.find(coord);
            if (it != pendingChunks.end()) {
                chunk = std::move(it->second);
                pendingChunks.erase(it);
                
                // Only add if not already loaded (prevent regeneration)
                if (chunks.count(coord) == 0) {
                    chunks[coord] = std::move(chunk);
                    wasAdded = true;
                }
            }
        }
        
        // Handle LRU operations outside mutex to avoid deadlock
        if (wasAdded) {
            touchChunk(coord);  // Add to LRU cache
        }
        
        // Create tiles for rendering
        if (chunks.count(coord) > 0) {
            const auto& chunkData = chunks[coord];
            
            std::cout << "DEBUG: Integrating chunk at world pos " << sphereToWorld(coord.centerOnSphere).x 
                      << ", " << sphereToWorld(coord.centerOnSphere).y 
                      << " (current chunk at " << sphereToWorld(currentChunk.centerOnSphere).x 
                      << ", " << sphereToWorld(currentChunk.centerOnSphere).y << ")" << std::endl;
            
            int debugCount = 0; // Reset for each chunk
            for (const auto& [localCoord, terrainData] : chunkData->tiles) {
                
                // Use pre-calculated game positions from ChunkGenerator
                // The ChunkGenerator has already calculated the final pixel positions
                // relative to the world origin. No complex transformations needed here.
                // See docs/ChunkedWorldImplementation.md for coordinate system details.
                
                // Get the pre-calculated game position (in pixels)
                glm::vec2 gamePos = terrainData.gamePosition;
                
                // Round to tile grid for consistent positioning
                int pixelX = static_cast<int>(std::round(gamePos.x / tileSize)) * static_cast<int>(tileSize);
                int pixelY = static_cast<int>(std::round(gamePos.y / tileSize)) * static_cast<int>(tileSize);
                
                WorldGen::TileCoord pixelCoord{pixelX, pixelY};
                
                // Debug first few tiles from this chunk
                if (debugCount < 5) {
                    std::cout << "  Tile " << localCoord.x << "," << localCoord.y 
                              << " game pos: " << gamePos.x << "," << gamePos.y
                              << " -> pixel: " << pixelX << "," << pixelY << std::endl;
                    debugCount++;
                }
                
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
                
                // Track which chunk this tile belongs to
                tileToChunkMap[pixelCoord] = coord;
                
                tilesCreated++;
            }
            
            std::cout << "DEBUG: Created " << tilesCreated << " tiles from chunk" << std::endl;
        }
    }
    
    // Tiles are integrated silently now that multi-chunk loading is working
    
    // Enforce chunk limit after all integrations are complete
    // This avoids deadlock by being outside the mutex lock
    enforceChunkLimit();
}

void World::updateTileVisibility() {
    auto& config = ConfigManager::getInstance();
    const int overscan = config.getTileCullingOverscan();
    const float tileSize = config.getTileSize();
    
    glm::vec4 bounds = getCameraBounds();
    
    
    // Get the range of tiles that should be visible
    auto [minX, maxX, minY, maxY] = getVisibleTileRange(overscan);
    
    
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
    
    
    // Reset chunk visibility tracking
    chunksWithVisibleTiles.clear();
    
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
                }
            } else if (it->second) {
                // Tile exists - just show it
                it->second->setVisible(true);
                tilesAlreadyExist++;
                
                // Track which chunk this visible tile belongs to
                auto chunkIt = tileToChunkMap.find(coord);
                if (chunkIt != tileToChunkMap.end()) {
                    chunksWithVisibleTiles[chunkIt->second]++;
                    touchChunk(chunkIt->second);  // Update LRU access time
                }
            }
        }
    }
    
    // Also track chunks for newly created tiles
    for (const auto& coord : newVisibleTiles) {
        if (tiles.count(coord) > 0) {
            auto chunkIt = tileToChunkMap.find(coord);
            if (chunkIt != tileToChunkMap.end()) {
                chunksWithVisibleTiles[chunkIt->second]++;
            }
        }
    }
    
    // Visibility updates are performed silently
    
    visibleTiles = std::move(newVisibleTiles);
    
    
    // Update camera state
    lastCameraPos = camera->getPosition();
    lastCameraBounds = glm::vec4(
        camera->getProjectionLeft(),
        camera->getProjectionRight(),
        camera->getProjectionBottom(),
        camera->getProjectionTop()
    );
    
    // Check if we need to load adjacent chunks
    checkAndLoadNearbyChunks();
    
    // COORDINATE FIX: Don't automatically switch current chunk
    // The current chunk defines our coordinate system origin (0,0).
    // Switching it would invalidate all tile positions since they're relative to current chunk.
    // Instead, we keep the initial chunk as the reference point and load tiles from
    // multiple chunks as needed.
    
    // Track which chunks have visible tiles for debugging/stats only
    if (!chunksWithVisibleTiles.empty()) {
        // Just update LRU for chunks with visible tiles
        for (const auto& [chunkCoord, visibleCount] : chunksWithVisibleTiles) {
            if (visibleCount > 0) {
                touchChunk(chunkCoord);
            }
        }
    }
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
    // Create tiles from loaded chunk data using pre-calculated positions
    // This replaces the old grid-based search system with direct position lookup
    // See docs/ChunkedWorldImplementation.md for coordinate system details
    
    auto& config = ConfigManager::getInstance();
    const float tileSize = config.getTileSize();
    int tilesCreated = 0;
    
    // Iterate through all loaded chunks and create tiles from their pre-calculated positions
    std::lock_guard<std::mutex> lock(chunkMutex);
    for (const auto& [chunkCoord, chunkData] : chunks) {
        if (!chunkData || !chunkData->isLoaded) {
            continue; // Skip unloaded chunks
        }
        
        // Create tiles from this chunk's terrain data
        for (const auto& [localCoord, terrainData] : chunkData->tiles) {
            // Use the pre-calculated game position from ChunkGenerator
            glm::vec2 gamePos = terrainData.gamePosition;
            
            // Round to tile grid for consistent positioning
            int pixelX = static_cast<int>(std::round(gamePos.x / tileSize)) * static_cast<int>(tileSize);
            int pixelY = static_cast<int>(std::round(gamePos.y / tileSize)) * static_cast<int>(tileSize);
            
            WorldGen::TileCoord pixelCoord{pixelX, pixelY};
            
            // Check if tile already exists (shouldn't happen during initial load)
            if (tiles.count(pixelCoord) > 0) {
                continue;
            }
            
            // Create tile at the pre-calculated position
            glm::vec2 tilePosForRendering(pixelX, pixelY);
            auto tile = std::make_shared<Rendering::Tile>(
                tilePosForRendering, terrainData.height, terrainData.resource, 
                terrainData.type
            );
            
            tiles[pixelCoord] = tile;
            worldLayer->addItem(tile);
            tile->setVisible(false); // Will be made visible by updateTileVisibility
            
            // Track which chunk this tile belongs to
            tileToChunkMap[pixelCoord] = chunkCoord;
            
            tilesCreated++;
        }
    }
    
    std::cout << "Created " << tilesCreated << " initial tiles from loaded chunks" << std::endl;
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
    
    // Find terrain data for this coordinate from ALL loaded chunks
    // The tile could be in any chunk, not just the current one
    std::lock_guard<std::mutex> lock(chunkMutex);
    
    
    for (const auto& [chunkCoord, chunkData] : chunks) {
        if (!chunkData) continue;
        
        // Convert pixel coordinate back to local chunk coordinate
        auto& config = ConfigManager::getInstance();
        const int chunkSize = config.getChunkSize();
        const float tileSize = config.getTileSize();
        
        // Calculate this chunk's position in world coordinates
        glm::vec2 thisChunkWorld = sphereToWorld(chunkCoord.centerOnSphere);
        glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
        glm::vec2 chunkOffset = thisChunkWorld - currentChunkWorld;
        
        // Convert chunk offset from meters to pixels
        // Must match the conversion in integrateLoadedChunks: 1 meter = 10 pixels
        const float tilesPerMeter = config.getTilesPerMeter();
        const float metersToPixels = tileSize * tilesPerMeter; // 10 * 1.0 = 10 pixels/meter
        glm::vec2 chunkOffsetPixels = chunkOffset * metersToPixels;
        
        // Convert pixel coordinate to local coordinate within this chunk
        int localX = static_cast<int>((coord.x - chunkOffsetPixels.x) / tileSize + chunkSize * 0.5f);
        int localY = static_cast<int>((coord.y - chunkOffsetPixels.y) / tileSize + chunkSize * 0.5f);
        
        // Check if this coordinate is within this chunk
        if (localX >= 0 && localX < chunkSize && localY >= 0 && localY < chunkSize) {
            WorldGen::TileCoord localCoord{localX, localY};
            auto terrainIt = chunkData->tiles.find(localCoord);
            
            
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
                
                // Track which chunk this tile belongs to
                tileToChunkMap[coord] = chunkCoord;
                
                
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
    
    gameState.set("world.loadedChunks", std::to_string(totalChunks));
    gameState.set("world.totalTiles", std::to_string(totalTiles));
    gameState.set("world.shownTiles", std::to_string(visibleTileCount));
    gameState.set("world.chunkMemKB", std::to_string(static_cast<int>(chunkMemoryKB)) + " KB");
    gameState.set("world.tileMemKB", std::to_string(static_cast<int>(tileMemoryKB)) + " KB");
    gameState.set("world.totalMemKB", std::to_string(static_cast<int>(totalMemoryKB)) + " KB");
}

void World::touchChunk(const WorldGen::Core::ChunkCoord& coord) {
    /**
     * Update the LRU access time for a chunk by moving it to the front of the list.
     * This ensures recently accessed chunks stay in memory.
     */
    
    // Only track chunks that are actually loaded
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        if (chunks.count(coord) == 0) {
            return;  // Don't track chunks that aren't loaded
        }
    }
    
    // If chunk is already tracked, remove it from its current position
    auto mapIt = chunkAccessMap.find(coord);
    if (mapIt != chunkAccessMap.end()) {
        chunkAccessOrder.erase(mapIt->second);
        chunkAccessMap.erase(mapIt);
    }
    
    // Add to front of list (most recently used)
    chunkAccessOrder.push_front(coord);
    chunkAccessMap[coord] = chunkAccessOrder.begin();
}

void World::enforceChunkLimit() {
    /**
     * Remove the oldest chunks if we exceed the configured limit.
     * This prevents memory from growing unbounded.
     */
    
    auto& config = ConfigManager::getInstance();
    const int maxChunks = config.getNumChunksToKeep();
    
    // Safety check: ensure we have more chunks than the limit
    if (chunkAccessOrder.size() <= static_cast<size_t>(maxChunks)) {
        return;
    }
    
    // Remove chunks from the back of the list (least recently used)
    while (chunkAccessOrder.size() > static_cast<size_t>(maxChunks)) {
        // Safety check: never remove the last chunk
        if (chunkAccessOrder.size() <= 1) {
            break;
        }
        
        WorldGen::Core::ChunkCoord oldestChunk = chunkAccessOrder.back();
        
        // Don't unload the current chunk - skip to next oldest
        if (oldestChunk == currentChunk) {
            // If we only have current chunk + 1 other, we can't remove anything
            if (chunkAccessOrder.size() <= 2) {
                break;
            }
            // Move current chunk to front
            chunkAccessOrder.pop_back();
            chunkAccessOrder.push_front(oldestChunk);
            chunkAccessMap[oldestChunk] = chunkAccessOrder.begin();
            continue;
        }
        
        // Remove from LRU tracking
        chunkAccessOrder.pop_back();
        chunkAccessMap.erase(oldestChunk);
        
        // Remove all tiles belonging to this chunk
        std::vector<WorldGen::TileCoord> tilesToRemove;
        for (const auto& [tileCoord, chunkCoord] : tileToChunkMap) {
            if (chunkCoord == oldestChunk) {
                tilesToRemove.push_back(tileCoord);
            }
        }
        
        for (const auto& tileCoord : tilesToRemove) {
            auto tileIt = tiles.find(tileCoord);
            if (tileIt != tiles.end()) {
                worldLayer->removeItem(tileIt->second);
                tiles.erase(tileIt);
                tileToChunkMap.erase(tileCoord);
                visibleTiles.erase(tileCoord);
            }
        }
        
        // Remove chunk data
        {
            std::lock_guard<std::mutex> lock(chunkMutex);
            chunks.erase(oldestChunk);
        }
        
        // Remove from visibility tracking
        chunksWithVisibleTiles.erase(oldestChunk);
    }
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

void World::checkAndLoadNearbyChunks() {
    /**
     * Check if the viewport is approaching chunk edges and trigger loading of adjacent chunks.
     * 
     * NOTE: Chunks are designed to be ~10x larger than the viewport, so players can pan 
     * around their starting location extensively before reaching chunk boundaries.
     * This is intended behavior - edge detection should only trigger after significant panning.
     */
    auto& config = ConfigManager::getInstance();
    const int chunkSize = config.getChunkSize();
    const float tileSize = config.getTileSize();
    const int edgeTriggerDistance = config.getChunkEdgeTriggerDistance();
    
    // Get visible tile range
    auto [minX, maxX, minY, maxY] = getVisibleTileRange(0); // No overscan
    
    // Convert to local tile coordinates within current chunk
    glm::vec2 currentChunkWorld = sphereToWorld(currentChunk.centerOnSphere);
    const float tilesPerMeter = config.getTilesPerMeter();
    
    // Calculate the bounds of the current chunk in pixel coordinates
    // The current chunk is centered at (0,0) in our local coordinate system
    // Tiles are positioned from -chunkSize/2 to chunkSize/2 in tile units
    // Convert to pixel coordinates by multiplying by tileSize
    int halfChunkPixels = static_cast<int>((chunkSize * 0.5f) * tileSize);
    int chunkMinX = -halfChunkPixels;
    int chunkMaxX = halfChunkPixels;
    int chunkMinY = -halfChunkPixels;
    int chunkMaxY = halfChunkPixels;
    
    // Calculate trigger distance in pixels
    int edgeTriggerPixels = edgeTriggerDistance * static_cast<int>(tileSize);
    
    // DEBUG: Log the coordinate systems
    static int debugCount = 0;
    if (debugCount++ < 5) {
        std::cout << "DEBUG: Current chunk world pos: (" << currentChunkWorld.x 
                  << ", " << currentChunkWorld.y << ") meters" << std::endl;
        std::cout << "DEBUG: Camera local pos: (" << camera->getPosition().x 
                  << ", " << camera->getPosition().y << ") pixels" << std::endl;
        std::cout << "DEBUG: Viewport range: [" << minX << "," << maxX 
                  << "] x [" << minY << "," << maxY << "] pixels" << std::endl;
        std::cout << "DEBUG: Chunk bounds: [" << chunkMinX << "," << chunkMaxX 
                  << "] x [" << chunkMinY << "," << chunkMaxY << "] pixels" << std::endl;
    }
    
    // Debug output occasionally (only when needed for debugging)
    // Static vars commented out to avoid any potential issues
    // static int debugCounter = 0;
    // if (++debugCounter % 60 == 0) {
    //     std::cout << "Edge check: viewport [" << minX << "," << maxX << "] x [" << minY << "," << maxY << "]" << std::endl;
    // }
    
    // Check if viewport is near chunk edges
    // We need to check if any visible tile is within edgeTriggerDistance tiles of a chunk edge
    
    bool needsAdjacent = false;
    std::vector<glm::ivec2> chunksToLoad;
    
    // Check each direction
    // Left edge: if leftmost visible tile (minX) is within trigger distance of left chunk edge
    if (minX < chunkMinX + edgeTriggerPixels) {
        chunksToLoad.push_back(glm::ivec2(-1, 0)); // Left
        needsAdjacent = true;
    }
    // Right edge: if rightmost visible tile (maxX) is within trigger distance of right chunk edge
    if (maxX > chunkMaxX - edgeTriggerPixels) {
        chunksToLoad.push_back(glm::ivec2(1, 0)); // Right
        needsAdjacent = true;
    }
    // Bottom edge: if bottommost visible tile (minY) is within trigger distance of bottom chunk edge
    if (minY < chunkMinY + edgeTriggerPixels) {
        chunksToLoad.push_back(glm::ivec2(0, -1)); // Bottom
        needsAdjacent = true;
    }
    // Top edge: if topmost visible tile (maxY) is within trigger distance of top chunk edge
    if (maxY > chunkMaxY - edgeTriggerPixels) {
        chunksToLoad.push_back(glm::ivec2(0, 1)); // Top
        needsAdjacent = true;
    }
    
    // Check corners if we're near two edges
    bool nearLeft = minX < chunkMinX + edgeTriggerPixels;
    bool nearRight = maxX > chunkMaxX - edgeTriggerPixels;
    bool nearBottom = minY < chunkMinY + edgeTriggerPixels;
    bool nearTop = maxY > chunkMaxY - edgeTriggerPixels;
    
    if (nearLeft && nearBottom) {
        chunksToLoad.push_back(glm::ivec2(-1, -1)); // Bottom-left
    }
    if (nearRight && nearBottom) {
        chunksToLoad.push_back(glm::ivec2(1, -1)); // Bottom-right
    }
    if (nearLeft && nearTop) {
        chunksToLoad.push_back(glm::ivec2(-1, 1)); // Top-left
    }
    if (nearRight && nearTop) {
        chunksToLoad.push_back(glm::ivec2(1, 1)); // Top-right
    }
    
    if (needsAdjacent) {
        // Load the needed chunks
        const float chunkSizeMeters = chunkSize / tilesPerMeter;
        
        for (const auto& offset : chunksToLoad) {
            // COORDINATE FIX: Calculate neighbor chunk properly
            // The offset is in chunk units (-1, 0, 1), multiply by chunk size in meters
            // to get the actual distance to the neighbor chunk center
            glm::vec2 neighborWorld = currentChunkWorld + glm::vec2(offset.x * chunkSizeMeters, offset.y * chunkSizeMeters);
            
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
                // Get timestamp
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
                
                std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") 
                         << "." << std::setfill('0') << std::setw(3) << ms.count() << "] "
                         << "Edge trigger: Requesting chunk at offset (" << offset.x << ", " << offset.y 
                         << ") world pos (" << static_cast<int>(neighborWorld.x) << ", " 
                         << static_cast<int>(neighborWorld.y) << ") - Camera at (" 
                         << camera->getPosition().x << ", " << camera->getPosition().y << ")" << std::endl;
                generateChunkAsync(neighborCoord);
            }
        }
    }
}