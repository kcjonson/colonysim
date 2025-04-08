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

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENGL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    C:/vcpkg/installed/x64-windows/include
    C:/vcpkg/installed/x64-windows/include/KHR
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    glm
    nlohmann_json::nlohmann_json
) 