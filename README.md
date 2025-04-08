# Colony Simulator

A 2D top-down colony simulation game inspired by Rimworld, built with C++ and OpenGL.

## Features

- Vector-based graphics rendering
- Real-time colony simulation
- Dynamic camera controls
- Shader-based visual effects

## Build Requirements

- CMake 3.5 or higher
- Python 3.6 or higher
- Git for Windows
- Visual Studio Build Tools 2022
  - MSVC v143 - VS 2022 C++ x64/x86 build tools
  - Windows 10/11 SDK
  - C++ CMake tools for Windows
- OpenGL 4.3 or higher
- GLFW 3.3 or higher

## Setup Guide

1. Install Git:
   - Download from https://git-scm.com/download/win
   - Run the installer with default settings
   - Verify installation: `git --version`

2. Install Python:
   - Download from https://www.python.org/downloads/
   - During installation, check "Add Python to PATH"
   - Verify installation: `python --version`

3. Install Visual Studio Build Tools:
   - Download from https://visualstudio.microsoft.com/visual-cpp-build-tools/
   - Select "Desktop development with C++"
   - Include:
     - MSVC v143 - VS 2022 C++ x64/x86 build tools
     - Windows 10/11 SDK
     - C++ CMake tools for Windows

4. Install vcpkg:
   - Open "Developer Command Prompt for VS 2022"
   - Run these commands:
     ```bash
     cd C:\
     git clone https://github.com/Microsoft/vcpkg.git
     cd vcpkg
     .\bootstrap-vcpkg.bat
     ```

5. Install required libraries using vcpkg:
   ```bash
   .\vcpkg install glfw3:x64-windows
   .\vcpkg install glad:x64-windows
   .\vcpkg integrate install
   ```

6. Build the project:
   - Open "Developer Command Prompt for VS 2022"
   - Navigate to project directory
   - Run:
     ```bash
     cmake -B build
     cmake --build build
     ```

## Project Structure

- `src/` - Source files
- `include/` - Header files
- `shaders/` - GLSL shader files
- `libs/` - Third-party libraries

## Controls

- Arrow Keys: Pan camera
- Mouse Wheel: Zoom in/out
- Edge Panning: Move mouse to screen edges

## Development

This project uses modern C++ features and follows these design principles:
- Component-based architecture
- Resource management through RAII
- Shader-based rendering pipeline
- Event-driven input system

## License

This project is licensed under the MIT License - see the LICENSE file for details. 