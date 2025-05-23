# Colony Simulator

A 2D top-down colony simulation game inspired by Rimworld, built with C++ and OpenGL.

## Features

- Vector-based graphics rendering
- Real-time colony simulation
- Dynamic camera controls
- Shader-based visual effects

## Build Requirements

- CMake 3.15 or higher
- Git
- C++ Compiler with C++20 support:
  - Windows: Visual Studio 2019/2022 with C++ workload
  - macOS: Xcode Command Line Tools or Clang
  - Linux: GCC 10+ or Clang 10+
- vcpkg package manager

## Setup Guide

1. Install Git:
   - Windows: Download from https://git-scm.com/download/win
   - macOS: `brew install git` or Xcode Command Line Tools
   - Linux: `sudo apt install git` or equivalent
   - Verify installation: `git --version`

2. Install a C++ compiler:
   - Windows: Install Visual Studio 2022 with "Desktop development with C++" workload
   - macOS: Install Xcode Command Line Tools: `xcode-select --install`
   - Linux: Install GCC: `sudo apt install build-essential` or equivalent

3. Install CMake:
   - Windows: Download from https://cmake.org/download/
   - macOS: `brew install cmake`
   - Linux: `sudo apt install cmake` or equivalent
   - Verify installation: `cmake --version`

4. Install vcpkg:
   ```bash
   # Windows (PowerShell)
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   
   # macOS/Linux
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh
   ```


5. Set VCPKG_ROOT environment variable and integrate
   ```bash
   # Windows (PowerShell)
   [Environment]::SetEnvironmentVariable("VCPKG_ROOT", "$PWD", [EnvironmentVariableTarget]::User)
   
   # macOS/Linux
   echo 'export VCPKG_ROOT="$PWD"' >> ~/.bashrc  # or .zshrc if using zsh
   source ~/.bashrc  # or source ~/.zshrc
   ```

   Integrate (IMPOTANT)
   ```
      $VCPKG_ROOT/vcpkg integrate install
   ```


6. Build the project:
   ```bash
   # Clone the repository if you haven't already
   git clone https://github.com/user/colonysim.git
   cd colonysim
   
   # Install dependencies using vcpkg manifest mode
   $VCPKG_ROOT/vcpkg install --triplet=x64-osx  # For macOS
   # $VCPKG_ROOT/vcpkg install --triplet=x64-windows  # For Windows
   # $VCPKG_ROOT/vcpkg install --triplet=x64-linux  # For Linux
   
   # Configure and build
   cmake -B build -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
   cmake --build build --config Release
   ```

   All dependencies specified in vcpkg.json will be automatically installed.

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