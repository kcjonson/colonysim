# Rendering Performance Unit Testing Plan

This document outlines our approach to creating a unit testing framework specifically for measuring and improving rendering performance in the ColonySim project.

## Testing Goals

1. **Performance Measurement**: Create baseline metrics to quantify rendering performance
2. **Regression Prevention**: Ensure optimizations don't break existing functionality
3. **Isolation**: Test rendering components in isolation without requiring the full game
4. **Reproducibility**: Generate consistent test scenarios for meaningful comparisons
5. **Modularity**: Allow tests to be run independently of the main application

## Testing Framework Implementation

We've implemented our testing framework using [Catch2](https://github.com/catchorg/Catch2) v3.x, a modern C++ test framework that provides several key benefits:
- Header-only library for easy integration
- Built-in benchmarking facilities with statistical analysis
- Minimal external dependencies
- Rich set of assertion macros
- Section-based test organization

## Directory Structure

The tests are organized with the following structure:

```
tests/                       # Main test directory
├── CMakeLists.txt           # CMake configuration for tests
├── main.cpp                 # Test runner main file
├── TestUtils.cpp            # Common test utilities implementation
├── TestUtils.h              # Common test utilities header
├── Mocks/                   # Mock implementations for testing
│   ├── MockGL.h             # OpenGL mocking header
│   └── MockGL.cpp           # OpenGL mocking implementation
├── Rendering/               # Tests for rendering components
│   ├── LayerTests.cpp       # Tests for Layer class
│   ├── TileTests.cpp        # Tests for Tile class
│   └── VectorRendererTests.cpp # Tests for VectorRenderer class
└── Performance/             # Performance-specific tests
    ├── RenderBenchmarks.cpp # Benchmarks for rendering operations
    ├── TileCullingTests.cpp # Tests for tile visibility culling
    └── ScalingTests.cpp     # Tests for performance at different scales
```

## Mock Dependencies

To ensure isolated testing of rendering components without requiring an actual OpenGL context or window, we've implemented the following mocks:

1. **MockOpenGL**: A static class that tracks OpenGL operations such as:
   - Draw call counts
   - Vertex counts
   - State changes (texture bindings, shader program switches)
   - Uniform updates

2. **TileTestCamera**: A simplified camera implementation for testing tile visibility culling with configurable view frustum bounds.

3. **MockLayer**: An extension of the Layer class specifically designed for testing, with additional methods to access protected members and tracking render calls.

4. **MockTile**: A simplified tile implementation that tracks rendering operations and supports visibility testing.

## Key Test Areas

### 1. Layer Management Tests

These tests validate the performance of layer management operations, focusing on:
- Adding and removing items from layers
- Sorting children by z-index
- Different sorting algorithm performance comparisons
- Rendering performance with different layer hierarchies (flat vs. deep vs. balanced)

### 2. Tile Rendering Tests

Tests for tile rendering performance, including:
- Basic tile rendering operations
- Grid rendering with different sizes (10x10, 32x32, 100x100, 250x250)
- Visibility determination performance
- View frustum culling with different view sizes
- Batching optimizations for grouped rendering
- Level of Detail (LOD) implementation for distant tiles

### 3. Vector Rendering Tests

Tests for vector graphics rendering performance:
- Individual vs. batched rendering comparisons
- Performance with different primitive types (rectangles, circles, lines)
- Complex UI rendering scenarios
- Color change state tracking

### 4. Performance Scaling Tests

Tests that analyze how rendering performance scales with:
- Increasing world sizes (from 10x10 to 1000x1000 tiles)
- Different rendering techniques (batched vs. unbatched)
- Frustum culling effectiveness
- Level of detail implementations
- Memory access patterns (row-major, texture-sorted, random)

## Test Data Generation

For reproducible testing, we've implemented several data generation functions:

1. **generateRandomLayers**: Creates a set of Layer objects with randomized z-index values using a fixed seed for reproducibility.

2. **createTileGrid**: Generates a grid of tiles with configurable dimensions and tile types.

3. **randomColor**: Provides deterministic "random" colors for visual elements.

## Performance Metrics Collection

Our test framework collects the following metrics:

1. **Draw call count**: Number of OpenGL draw operations
2. **Vertex count**: Number of vertices processed
3. **State change count**: Number of OpenGL state changes (shader switches, texture binds)
4. **Execution time**: Precise timing of operations using Catch2's benchmarking system
5. **Operation counts**: For tracking specific operations like sorting or culling

## Build Integration

Tests are integrated into the project's CMake build system with dedicated tasks:

1. **build-tests**: Builds just the test executable
2. **run-tests**: Builds and runs the tests, showing regular test results
3. **run-benchmarks**: Can be used to run just the benchmark tests with custom sample counts

## Future Improvements

1. Add memory allocation tracking to monitor heap usage during rendering
2. Implement automated performance regression detection
3. Add visual difference testing for rendering output
4. Expand testing to cover more components (text rendering, particles)
5. Add multi-threading performance tests