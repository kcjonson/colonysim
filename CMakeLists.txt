cmake_minimum_required(VERSION 3.10)
project(ColonySim)

# Set vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(freetype REQUIRED)

# Add source files
set(SOURCES
    src/Camera.cpp
    src/ConfigManager.cpp
    src/Entity.cpp
    src/EntityManager.cpp
    src/Examples.cpp
    src/FontRenderer.cpp
    src/Game.cpp
    src/InputManager.cpp
    src/Interface.cpp
    src/main.cpp
    src/Renderer.cpp
    src/Rendering/Layer.cpp
    src/Rendering/Shapes/Circle.cpp
    src/Rendering/Shapes/Line.cpp
    src/Rendering/Shapes/Polygon.cpp
    src/Rendering/Shapes/Rectangle.cpp
    src/Rendering/Shapes/Shape.cpp
    src/Rendering/Shapes/Text.cpp
    src/Rendering/Styles/Base.cpp
    src/Rendering/Styles/Border.cpp
    src/Rendering/Styles/Shape.cpp
    src/Shader.cpp
    src/Tile.cpp
    src/VectorGraphics.cpp
    src/VectorRenderer.cpp
    src/World.cpp
)

# Add header files
set(HEADERS
    include/Camera.h
    include/ConfigManager.h
    include/Entity.h
    include/EntityManager.h
    include/Examples.h
    include/FontRenderer.h
    include/Game.h
    include/GameState.h
    include/InputManager.h
    include/Interface.h
    include/Renderer.h
    include/Rendering/Layer.h
    include/Rendering/Shapes/Circle.h
    include/Rendering/Shapes/Line.h
    include/Rendering/Shapes/Polygon.h
    include/Rendering/Shapes/Rectangle.h
    include/Rendering/Shapes/Shape.h
    include/Rendering/Shapes/Text.h
    include/Rendering/Styles/Base.h
    include/Rendering/Styles/Border.h
    include/Rendering/Styles/Shape.h
    include/Shader.h
    include/Tile.h
    include/Vertex.h
    include/VectorGraphics.h
    include/VectorRenderer.h
    include/World.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Copy config to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/config
)

# Copy fonts to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    C:/vcpkg/installed/x64-windows/include
    C:/vcpkg/installed/x64-windows/include/KHR
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    glm
    nlohmann_json::nlohmann_json
    freetype
) 