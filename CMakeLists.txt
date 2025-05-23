cmake_minimum_required(VERSION 3.10)
project(ColonySim)

# Detect platform for vcpkg integration and setting triplet
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
elseif(APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
elseif(UNIX AND NOT APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
endif()

message(STATUS "Using triplet: ${VCPKG_TARGET_TRIPLET}")

# Check for local vcpkg installation
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share")
    # Add local vcpkg packages to the prefix path
    list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share")
    message(STATUS "Using local vcpkg packages: ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share")
endif()

# Set vcpkg toolchain file if VCPKG_ROOT is defined
if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    message(STATUS "Using vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")
endif()

# Debugging messages to verify vcpkg integration
message(STATUS "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
message(STATUS "VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")


# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(freetype REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)


# Add source files
# Using file(GLOB) to find source files automatically
file(GLOB_RECURSE SOURCES 
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(GLOB_RECURSE HEADERS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp")

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Add compiler-specific flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/std:c++20" "/Zc:__cplusplus")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE "-std=c++20")
endif()

# Add GLM-specific definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    GLM_FORCE_CXX17
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_PURE)

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)

# Copy Planet shaders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/Planet
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/Planet
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${OPENGL_INCLUDE_DIR}
    ${GLM_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OPENGL_LIBRARIES}
    glfw
    nlohmann_json::nlohmann_json
    freetype
    glad::glad
)

# Add the tests subdirectory
add_subdirectory(tests)