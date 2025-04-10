cmake_minimum_required(VERSION 3.10)
project(ColonySim)

# Set vcpkg toolchain file
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(freetype REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Entity.cpp
    src/EntityManager.cpp
    src/World.cpp
    src/Camera.cpp
    src/InputManager.cpp
    src/VectorGraphics.cpp
    src/ConfigManager.cpp
    src/Interface.cpp
    src/FontRenderer.cpp
    # Add new rendering files
    src/Rendering/Layer.cpp
    src/Rendering/Shapes/Shape.cpp
    src/Rendering/Shapes/Rectangle.cpp
    src/Rendering/Shapes/Circle.cpp
    src/Rendering/Shapes/Line.cpp
    src/Rendering/Shapes/Polygon.cpp
    src/Rendering/Shapes/Text.cpp
    src/Tile.cpp
)

# Add header files
set(HEADERS
    include/Game.h
    include/Entity.h
    include/EntityManager.h
    include/World.h
    include/Camera.h
    include/InputManager.h
    include/VectorGraphics.h
    include/ConfigManager.h
    include/Interface.h
    include/FontRenderer.h
    # Add new rendering headers
    include/Rendering/Layer.h
    include/Rendering/Shapes/Shape.h
    include/Rendering/Shapes/Rectangle.h
    include/Rendering/Shapes/Circle.h
    include/Rendering/Shapes/Line.h
    include/Rendering/Shapes/Polygon.h
    include/Rendering/Shapes/Text.h
    include/Tile.h
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