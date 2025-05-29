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

**CRITICAL DESIGN NOTE**: This is the most complex part of the system and has been a major source of bugs. The coordinate transformations must be used consistently throughout the codebase. See the coordinate conversion functions in `src/Screens/WorldGen/Core/Util.h` for the canonical implementations.

The implementation uses a carefully designed 4-tier coordinate system to handle the massive scale of a planet while maintaining precision:

### 1. Sphere Coordinates (3D) - Primary Storage
- **Format**: Normalized 3D vectors on unit sphere (x,y,z where x²+y²+z² = 1)
- **Purpose**: Primary storage format for all positions on the planet
- **Advantages**: 
  - Mathematically clean (no distortion)
  - Natural for sphere calculations
  - Consistent precision everywhere on planet
- **Usage**: 
  - Chunk centers (`ChunkCoord.centerOnSphere`)
  - Landing locations
  - Any long-term position storage
- **Coordinate Convention**:
  - (1,0,0) = Prime meridian (0°) and Equator (0°) - **WORLD ORIGIN**
  - (0,1,0) = North pole (90° latitude)
  - (0,0,1) = 90° east longitude on equator
  - (-1,0,0) = 180° longitude (international date line)

### 2. World Coordinates (2D) - Spatial Calculations
- **Format**: Linear distances in meters from world origin (x,y)
- **Purpose**: Intermediate system for spatial calculations and chunk positioning
- **Conversion**: Longitude/latitude in radians × planet radius (6,371,000m)
- **Coordinate System**:
  - Origin (0,0) = Prime meridian and equator (sphere position 1,0,0)
  - X-axis = East/west distance in meters from prime meridian
  - Y-axis = North/south distance in meters from equator
- **Distortion**: Uses equirectangular projection (distortion increases near poles)
- **Rationale**: 
  - Avoids floating-point precision issues with planet-scale pixel coordinates
  - Intuitive for chunk spacing ("chunks are 400 meters apart")
  - Manageable numbers for spatial calculations
- **Scale Consideration**: At planet scale, coordinates can reach ±20 million meters (half Earth's circumference)

### 3. Game Coordinates (2D) - Local Rendering
- **Format**: Pixel coordinates relative to current view area
- **Purpose**: Final coordinates for tile rendering and camera positioning
- **Conversion**: World meters × (tiles per meter × pixels per tile) = World meters × 10
- **Local Origin**: Typically centered on current play area to maintain precision
- **Rationale**:
  - Prevents trillion-pixel coordinates that would cause floating-point errors
  - Natural units for rendering system
  - Allows precise tile positioning and smooth camera movement

### 4. Chunk Local Coordinates - Per-Chunk Indexing
- **Format**: Integer tile indices within a chunk (0 to chunkSize-1)
- **Purpose**: Indexing tiles within individual chunks
- **Size**: 400×400 tiles per chunk (configurable)
- **Coordinate Range**: (0,0) to (399,399) for default 400×400 chunks
- **Conversion to World**: Requires chunk center position + local offset calculation

## Coordinate Transformation Chain

The complete transformation chain for positioning a tile:

```
Sphere Position (storage)
    ↓ [sphereToWorld()]
World Coordinates (meters)
    ↓ [worldToGame()]  
Game Coordinates (pixels)
    ↓ [tile rendering]
Screen Position
```

**Key Functions** (in `src/Screens/WorldGen/Core/Util.h`):
- `sphereToLatLong()`: Foundation function, converts sphere→longitude/latitude
- `sphereToWorld()`: Converts sphere→world coordinates  
- `worldToGame()`: Converts world→game coordinates

## Critical Design Constraints

### Scale Management
- **Problem**: Planet-scale worlds create coordinates in trillions of pixels
- **Solution**: Use world coordinates (meters) as intermediate system
- **Benefit**: Keeps game coordinates manageable and precise

### Polar Distortion
- **Problem**: Equirectangular projection distorts distances near poles
- **Mitigation**: Landing locations restricted to ±60° latitude
- **Acceptable Trade-off**: Slight distance errors acceptable for gameplay

### Floating-Point Precision
- **Problem**: Large coordinates lose precision in floating-point arithmetic
- **Solution**: Multi-tier system keeps each coordinate range reasonable
- **Implementation**: Game coordinates stay local, world coordinates handle planet scale

### Consistency Requirements
- **Critical**: All coordinate conversions MUST use the shared utility functions
- **Enforcement**: Code comments reference this documentation
- **Testing**: Verify chunk boundaries align perfectly (no gaps or overlaps)

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