# Chunked World Implementation

## Overview

This document describes the chunked world generation system implemented for ColonySim. The system allows for infinite world exploration by generating terrain chunks on-demand as the player moves around the planet.

## Key Design Decisions

### 1. Global Coordinate System

**Decision**: Use a fixed global origin at the equator/prime meridian intersection (sphere point (1,0,0))

**Rationale**:
- Consistent coordinates across the entire planet
- Enables teleportation between distant locations
- Allows coordinate sharing between players
- Avoids confusion from landing-relative coordinates

**Implementation**:
- Origin (0,0) is at equator/prime meridian
- X axis points east along the equator
- Y axis points north toward the north pole
- Units are in meters (distance along sphere surface)

### 2. Chunk Indexing by Sphere Position

**Decision**: Chunks are indexed by their center position on the unit sphere (3D coordinates)

**Rationale**:
- Chunks can be generated independently at any location
- Supports non-adjacent chunk generation (teleportation)
- Consistent chunk generation regardless of access pattern
- Natural mapping from sphere to chunks

**Implementation**:
- `ChunkCoord` stores a normalized 3D vector (center on sphere)
- Each chunk has its own local tangent plane projection
- Chunks are generated on a regular grid in world space

### 3. Separation of Concerns

**Decision**: Separate chunk generation logic from world rendering

**Components**:
- `ChunkGenerator`: Handles sphere sampling and terrain generation
- `ChunkedWorld`: Manages chunk loading/unloading and rendering
- `ChunkData`: Pure data structure for chunk information

**Benefits**:
- Reusable chunk generation for different contexts
- Clear separation of coordinate systems
- Easier testing and maintenance

## Coordinate Systems

The implementation uses multiple coordinate systems:

1. **Sphere Coordinates (3D)**
   - Normalized vectors on unit sphere
   - Used for chunk indexing and world generation

2. **World Coordinates (2D)**
   - Global 2D coordinates in meters
   - Origin at equator/prime meridian
   - Used for player position and distance calculations

3. **Chunk Local Coordinates**
   - Tile indices within a chunk (0 to CHUNK_SIZE-1)
   - Each chunk has its own local projection

4. **Screen Coordinates**
   - Camera-relative coordinates for rendering

## Configuration

World generation parameters are stored in `config/game_config.json`:

```json
"world": {
    "chunkSize": 1000,           // Tiles per chunk
    "tileSize": 20.0,            // Size of each tile in pixels
    "tilesPerMeter": 1.0,        // Sampling density
    "preloadRadius": 1,          // Chunks to preload around player
    "unloadRadius": 2,           // Distance to unload chunks
    "maxLoadedChunks": 9,        // Maximum chunks in memory
    "maxNewTilesPerFrame": 100,  // Performance limit
    "tileCullingOverscan": 3     // Extra tiles to render off-screen
}
```

## Implementation Flow

1. **Initialization**
   - Player lands at a location on the sphere
   - Initial chunk is generated during WorldGen phase
   - Player position is calculated in world coordinates
   - Camera is positioned at player location

2. **Chunk Loading**
   - As player moves, current chunk is updated
   - Adjacent chunks are queued for generation
   - Background thread generates chunks using ChunkGenerator
   - Main thread integrates completed chunks

3. **Rendering**
   - Only visible tiles within loaded chunks are rendered
   - Tile visibility is culled based on camera bounds
   - Tiles are positioned using world coordinates

## Future Enhancements

1. **Spatial Indexing**: Replace linear search in `findNearestTile()` with octree or similar
2. **Level of Detail**: Different detail levels for distant chunks
3. **Chunk Compression**: Store only non-default tiles
4. **Persistent Storage**: Save/load chunks from disk
5. **Improved Projections**: Better handling of polar regions

## Performance Considerations

- Chunks are generated asynchronously to avoid frame drops
- Tile creation is limited per frame (maxNewTilesPerFrame)
- Distant chunks are unloaded to manage memory
- Visibility culling reduces rendering load