cmake_minimum_required(VERSION 3.15)

# Find fmt package
find_package(fmt CONFIG REQUIRED)

# Include source files needed from the main project
set(SOURCE_FILES
    # Removed Layer.cpp to avoid duplicate definition
    ${CMAKE_SOURCE_DIR}/src/VectorGraphics.cpp # Added missing dependency
    ${CMAKE_SOURCE_DIR}/src/Camera.cpp         # Added missing dependency
    ${CMAKE_SOURCE_DIR}/src/Shader.cpp         # Added missing dependency for VectorGraphics
    ${CMAKE_SOURCE_DIR}/src/Renderer.cpp       # Added missing dependency for VectorGraphics
    ${CMAKE_SOURCE_DIR}/src/VectorRenderer.cpp # Added missing dependency for Rendering::Draw
    ${CMAKE_SOURCE_DIR}/src/FontRenderer.cpp   # Added missing dependency for Renderer
    ${CMAKE_SOURCE_DIR}/src/Rendering/Draw/Circle.cpp    # Added missing dependency for VectorGraphics
    ${CMAKE_SOURCE_DIR}/src/Rendering/Draw/Line.cpp      # Added missing dependency for VectorGraphics
    ${CMAKE_SOURCE_DIR}/src/Rendering/Draw/Polygon.cpp   # Added missing dependency for VectorGraphics
    ${CMAKE_SOURCE_DIR}/src/Rendering/Draw/Rectangle.cpp # Added missing dependency for VectorGraphics
)

# Explicitly list test source files relative to the current CMakeLists.txt
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TestUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Mocks/MockGL.cpp
    # Removed missing files: MockVectorGraphics.cpp, MockCamera.cpp, LayerPerformanceTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/RenderBenchmarks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/TileCullingTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Performance/ScalingTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/VectorRendererTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/TileTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Rendering/LayerTests.cpp
)

# Create test executable - using SOURCE_FILES to include real implementations
add_executable(ColonySimTests ${TEST_SOURCES} ${SOURCE_FILES})

# Use C++17 standard
set_target_properties(ColonySimTests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Define TESTING macro for test builds
target_compile_definitions(ColonySimTests PRIVATE TESTING=1)

# Find necessary packages
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)  # Add fmt library dependency

# Include directories
target_include_directories(ColonySimTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/extern
    ${CMAKE_SOURCE_DIR}
)

# Link with dependencies
target_link_libraries(ColonySimTests PRIVATE
    glm::glm
    ${OPENGL_LIBRARIES}
    Catch2::Catch2
    fmt::fmt  # Link against fmt library
)

# Add custom target to run tests
add_custom_target(run_tests
    COMMAND ColonySimTests
    DEPENDS ColonySimTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ColonySim tests"
)

# Add custom target to run only benchmark tests
add_custom_target(run_benchmarks
    COMMAND ColonySimTests "[benchmark]" --benchmark-samples 100
    DEPENDS ColonySimTests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running ColonySim benchmarks"
)

# Copy necessary test data files to build directory if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/data)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR}/tests)
endif()