#include "CoordinateSystem.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// COORDINATE SYSTEM DESIGN PHILOSOPHY:
// 
// This coordinate system is designed to abstract away the complexity of high-DPI displays
// (like Retina displays on macOS) from the rest of the application.
//
// Key concepts:
// 1. LOGICAL PIXELS (Window Coordinates): What the user works with. These are the same
//    regardless of display DPI. A 100x100 button is always 100x100 logical pixels.
//
// 2. PHYSICAL PIXELS (Framebuffer Coordinates): Actual pixels on the screen. On a 2x
//    Retina display, a 100x100 logical pixel button is 200x200 physical pixels.
//
// 3. PIXEL RATIO: The ratio between physical and logical pixels (e.g., 2.0 on Retina).
//
// Design decisions:
// - All public APIs use logical pixels (window coordinates)
// - Only glViewport uses physical pixels (framebuffer size)
// - Projection matrices use logical pixels to keep consistent coordinate spaces
// - Mouse input is already in logical pixels from GLFW
// - This abstraction is hidden from UI components - they just use logical pixels

glm::mat4 CoordinateSystem::createScreenSpaceProjection() const {
    // IMPORTANT: We use window size (logical pixels) for projection matrices, NOT framebuffer size.
    // This ensures UI elements have consistent sizes regardless of display DPI.
    // The GPU will handle the scaling to physical pixels automatically.
    int width, height;
    if (window) {
        glfwGetWindowSize(window, &width, &height);
    } else {
        width = 1920;  // Default fallback
        height = 1080;
    }
    
    // Screen-space orthographic projection: (0,0) at top-left, Y increases downward
    return glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, -1.0f, 1.0f);
}

glm::mat4 CoordinateSystem::createWorldSpaceProjection() const {
    // Use window size for consistency with screen space projection
    int width, height;
    if (window) {
        glfwGetWindowSize(window, &width, &height);
    } else {
        width = 1920;  // Default fallback
        height = 1080;
    }
    
    // World-space orthographic projection: (0,0) at center, Y increases upward
    float halfWidth = static_cast<float>(width) / 2.0f;
    float halfHeight = static_cast<float>(height) / 2.0f;
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
}

glm::vec2 CoordinateSystem::getWindowSize() const {
    // Return logical window size, not physical framebuffer size
    int width, height;
    if (window) {
        glfwGetWindowSize(window, &width, &height);
    } else {
        width = 1920;  // Default fallback
        height = 1080;
    }
    return glm::vec2(static_cast<float>(width), static_cast<float>(height));
}

void CoordinateSystem::resetOpenGLState() const {
    // Reset OpenGL state to defaults for 2D rendering
    glDisable(GL_DEPTH_TEST);  // Disable depth testing for 2D
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

bool CoordinateSystem::initialize(GLFWwindow* win) {
    window = win;
    return window != nullptr;
}

void CoordinateSystem::setFullViewport() const {
    // IMPORTANT: glViewport needs PHYSICAL pixels (framebuffer size), not logical pixels!
    // This is the only place where we use framebuffer size instead of window size.
    if (window) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }
}

void CoordinateSystem::updateWindowSize(int width, int height) {
    // Window size is tracked by GLFW, nothing to store here
    // This method exists for API compatibility
    // Mark pixel ratio as dirty so it gets recalculated
    pixelRatioDirty = true;
}

float CoordinateSystem::getPixelRatio() const {
    // Calculate and cache the pixel ratio for performance
    if (pixelRatioDirty && window) {
        int windowWidth, windowHeight;
        int framebufferWidth, framebufferHeight;
        
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        
        // Use the width ratio (should be same as height ratio)
        if (windowWidth > 0) {
            cachedPixelRatio = static_cast<float>(framebufferWidth) / static_cast<float>(windowWidth);
        } else {
            cachedPixelRatio = 1.0f;
        }
        
        pixelRatioDirty = false;
    }
    
    return cachedPixelRatio;
}

glm::vec2 CoordinateSystem::windowToFramebuffer(const glm::vec2& windowCoords) const {
    // Convert logical pixels to physical pixels
    float ratio = getPixelRatio();
    return windowCoords * ratio;
}

glm::vec2 CoordinateSystem::framebufferToWindow(const glm::vec2& fbCoords) const {
    // Convert physical pixels to logical pixels
    float ratio = getPixelRatio();
    return fbCoords / ratio;
}

float CoordinateSystem::percentWidth(float percent) const {
    // Convert percentage to logical pixels
    glm::vec2 size = getWindowSize();
    return size.x * (percent / 100.0f);
}

float CoordinateSystem::percentHeight(float percent) const {
    // Convert percentage to logical pixels
    glm::vec2 size = getWindowSize();
    return size.y * (percent / 100.0f);
}

glm::vec2 CoordinateSystem::percentSize(float widthPercent, float heightPercent) const {
    // Convert percentage dimensions to logical pixels
    glm::vec2 size = getWindowSize();
    return glm::vec2(
        size.x * (widthPercent / 100.0f),
        size.y * (heightPercent / 100.0f)
    );
}

glm::vec2 CoordinateSystem::percentPosition(float xPercent, float yPercent) const {
    // Convert percentage position to logical pixels
    glm::vec2 size = getWindowSize();
    return glm::vec2(
        size.x * (xPercent / 100.0f),
        size.y * (yPercent / 100.0f)
    );
}