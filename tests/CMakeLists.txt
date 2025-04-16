cmake_minimum_required(VERSION 3.15)

# Set fmt library location explicitly
set(fmt_DIR "C:/vcpkg/installed/x64-windows/share/fmt")

# Include source files needed from the main project
set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/Rendering/Layer.cpp
)

# Explicitly list test source files
set(TEST_SOURCES
    main.cpp
    TestUtils.cpp
    Mocks/MockGL.cpp
    Mocks/MockVectorGraphics.cpp
    Mocks/MockCamera.cpp
    Performance/LayerPerformanceTests.cpp
    Performance/RenderBenchmarks.cpp
    Performance/TileCullingTests.cpp
    Performance/ScalingTests.cpp
    Rendering/VectorRendererTests.cpp
    Rendering/TileTests.cpp
    Rendering/LayerTests.cpp
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
    C:/vcpkg/installed/x64-windows/include # For vcpkg-installed packages
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