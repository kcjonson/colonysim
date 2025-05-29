#pragma once

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <list>
#include <glm/glm.hpp>
#include "../../VectorGraphics.h"
#include "../../Camera.h"
#include "../../Rendering/Layer.h"
#include "../../GameState.h"
#include "../WorldGen/Core/TerrainTypes.h"
#include "../WorldGen/Core/ChunkTypes.h"
#include "../WorldGen/Generators/World.h"
#include "Tile.h"

class World {
public:
    World(GameState& gameState, 
          const std::string& seed, 
          Camera* camera, 
          GLFWwindow* window,
          const WorldGen::Generators::World* sphericalWorld,
          std::unique_ptr<WorldGen::Core::ChunkData> initialChunk,
          const glm::vec3& landingLocation);
    ~World();

    // Initialize world (sets up rendering, starts background thread)
    bool initialize();
    
    // Update world state
    void update(float deltaTime);
    
    // Render visible chunks
    void render();

private:
    // Core data
    GameState& gameState;
    std::string seed;
    Camera* camera;
    const WorldGen::Generators::World* sphericalWorld; // The 3D world we sample from
    
    // Chunk management
    std::unordered_map<WorldGen::Core::ChunkCoord, std::unique_ptr<WorldGen::Core::ChunkData>> chunks;
    std::unordered_map<WorldGen::Core::ChunkCoord, std::unique_ptr<WorldGen::Core::ChunkData>> pendingChunks;
    WorldGen::Core::ChunkCoord currentChunk;
    std::mutex chunkMutex;
    
    // Rendering
    std::shared_ptr<Rendering::Layer> worldLayer;
    std::unordered_map<WorldGen::TileCoord, std::shared_ptr<Rendering::Tile>> tiles;
    std::unordered_set<WorldGen::TileCoord> visibleTiles;
    
    // Track which chunks have visible tiles (chunk coord -> visible tile count)
    std::unordered_map<WorldGen::Core::ChunkCoord, int> chunksWithVisibleTiles;
    
    // Track which chunk each tile belongs to
    std::unordered_map<WorldGen::TileCoord, WorldGen::Core::ChunkCoord> tileToChunkMap;
    
    // LRU chunk cache - tracks access order for memory management
    // Most recently accessed chunks are at the front
    std::list<WorldGen::Core::ChunkCoord> chunkAccessOrder;
    // Quick lookup from chunk coord to position in the access list
    std::unordered_map<WorldGen::Core::ChunkCoord, std::list<WorldGen::Core::ChunkCoord>::iterator> chunkAccessMap;
    
    // World configuration
    
    /**
     * Player's current position in GLOBAL WORLD COORDINATES (meters).
     * 
     * COORDINATE SYSTEM: Global 2D world space
     * - Origin (0,0) is at the equator/prime meridian intersection (sphere point (1,0,0))
     * - X axis points east along the equator
     * - Y axis points north along the prime meridian toward the north pole
     * - Units are in meters (distance along sphere surface)
     * 
     * DESIGN DECISION: We use a fixed global origin rather than a landing-relative
     * origin to ensure consistent coordinates across the entire planet. This allows:
     * - Teleportation between distant locations
     * - Sharing coordinates between players
     * - Consistent chunk indexing regardless of where you started
     * 
     * This position is used to:
     * - Determine which chunk the player is in
     * - Calculate which chunks need to be loaded/unloaded
     * - Position the camera for rendering
     * 
     * To convert to other coordinate systems:
     * - Chunk space: Use worldToChunk() and worldToLocalTile()
     * - Sphere space: Use worldToSphere() to get 3D position
     * - Screen space: Apply camera transformation
     */
    glm::vec2 playerPosition;
    
    /**
     * The player's landing location on the sphere.
     * 
     * COORDINATE SYSTEM: 3D unit sphere
     * - Normalized 3D vector pointing to location on sphere surface
     * - This is where the player started the game
     * - Used to initialize the player's starting position in world coordinates
     */
    glm::vec3 landingLocation;
    
    // Background generation
    std::thread chunkGeneratorThread;
    std::queue<WorldGen::Core::ChunkCoord> chunkLoadQueue;
    std::condition_variable chunkCondVar;
    bool running = true;
    
    // Camera tracking
    glm::vec3 lastCameraPos = glm::vec3(0.0f);
    glm::vec4 lastCameraBounds = glm::vec4(0.0f);
    
    // Performance tracking
    float timeSinceLastLog = 0.0f;
    
    // Methods
    void updateCurrentChunk();
    void loadAdjacentChunks();
    void unloadDistantChunks();
    void integrateLoadedChunks();
    void touchChunk(const WorldGen::Core::ChunkCoord& coord);  // Update LRU access time
    void enforceChunkLimit();  // Remove oldest chunks if over limit
    
    void generateChunk(const WorldGen::Core::ChunkCoord& coord);
    void generateChunkAsync(const WorldGen::Core::ChunkCoord& coord);
    void chunkGeneratorThreadFunc();
    
    void checkAndLoadNearbyChunks();
    
    void updateTileVisibility();
    bool cameraViewChanged() const;
    glm::vec4 getCameraBounds() const;
    
    void logMemoryUsage() const;
    
    /**
     * Calculate the range of tile coordinates that should be visible on screen.
     * @param overscan Additional tiles to include beyond the visible area (for preloading)
     * @return Tuple of (minX, maxX, minY, maxY) in pixel coordinates
     */
    std::tuple<int, int, int, int> getVisibleTileRange(int overscan) const;
    
    /**
     * Create the initial set of tiles needed to fill the window plus preload radius.
     * This ensures tiles are ready for rendering before the first frame.
     */
    void createInitialTiles();
    
    /**
     * Create a tile instance from terrain data for the given coordinate.
     * @param coord Tile coordinate in pixel space
     * @return true if tile was successfully created, false if no terrain data found
     */
    bool createTileFromData(const WorldGen::TileCoord& coord);
    
    // Coordinate system conversions
    // All methods clearly document their input/output coordinate systems
    
    /**
     * Convert from WORLD COORDINATES to sphere coordinates for a chunk.
     * @param worldPos Position in 2D world space (meters from origin)
     * @return ChunkCoord containing the sphere position for the chunk containing this world position
     */
    WorldGen::Core::ChunkCoord worldToChunk(const glm::vec2& worldPos) const;
    
    /**
     * Convert from WORLD COORDINATES to local tile coordinates within a chunk.
     * @param worldPos Position in 2D world space (meters from origin)
     * @return Local tile coordinates (0 to CHUNK_SIZE-1) within the containing chunk
     */
    WorldGen::TileCoord worldToLocalTile(const glm::vec2& worldPos) const;
    
    /**
     * Convert from CHUNK + LOCAL TILE coordinates to world coordinates.
     * @param chunk The chunk (identified by its sphere position)
     * @param localTile Local tile position within that chunk (0 to CHUNK_SIZE-1)
     * @return Position in 2D world space (meters from origin)
     */
    glm::vec2 tileToWorld(const WorldGen::Core::ChunkCoord& chunk, const WorldGen::TileCoord& localTile) const;
    
    /**
     * Convert from WORLD COORDINATES to sphere coordinates.
     * 
     * DESIGN NOTE: This conversion uses a cylindrical projection where:
     * - X coordinate represents distance east/west along great circles
     * - Y coordinate represents distance north/south along meridians
     * 
     * @param worldPos Position in 2D world space (meters from origin at equator/prime meridian)
     * @return 3D position on unit sphere
     */
    glm::vec3 worldToSphere(const glm::vec2& worldPos) const;
    
    /**
     * Convert from SPHERE COORDINATES to world coordinates.
     * 
     * @param spherePos 3D position on unit sphere
     * @return Position in 2D world space (meters from origin at equator/prime meridian)
     */
    glm::vec2 sphereToWorld(const glm::vec3& spherePos) const;
    
    // Constants for world projection
    static constexpr float PLANET_RADIUS = 6371000.0f;  // Earth radius in meters
};