#pragma once

#include <glm/glm.hpp>

// Forward declaration to avoid OpenGL header conflicts
struct GLFWwindow;

/**
 * CoordinateSystem manages window coordinates, DPI scaling, and viewport handling
 * for consistent rendering across different screens and displays on macOS.
 */
class CoordinateSystem {
public:
    static CoordinateSystem& getInstance();
    
    // Initialize with window - must be called after GLFW window creation
    bool initialize(GLFWwindow* window);
    
    // Update when window size changes
    void updateWindowSize(int windowWidth, int windowHeight);
    
    // Get window dimensions in different coordinate systems
    struct WindowDimensions {
        int windowWidth;     // Window size in screen coordinates
        int windowHeight;
        int framebufferWidth;  // Framebuffer size in pixels
        int framebufferHeight;
        float scaleX;        // DPI scale factors
        float scaleY;
    };
    
    const WindowDimensions& getDimensions() const { return dimensions; }
    
    // Coordinate conversions
    glm::vec2 screenToFramebuffer(const glm::vec2& screenCoords) const;
    glm::vec2 framebufferToScreen(const glm::vec2& framebufferCoords) const;
    
    // Create projection matrices for different use cases
    glm::mat4 createScreenSpaceProjection() const;      // UI elements (0,0 = top-left)
    glm::mat4 createWorldSpaceProjection() const;       // World content (0,0 = center)
    glm::mat4 createCenteredProjection() const;         // Centered coordinate system
    
    // Viewport management
    void setFullViewport() const;
    void setViewport(int x, int y, int width, int height) const;
    void setScaledViewport(float x, float y, float width, float height) const;
    
    // OpenGL state management for screen transitions
    void resetOpenGLState() const;
    void saveOpenGLState();
    void restoreOpenGLState() const;
    
    // Utility functions for UI layout
    float getAspectRatio() const;
    glm::vec2 getWindowCenter() const;
    glm::vec2 getWindowSize() const;
    
    // For backward compatibility and gradual migration
    int getWindowWidth() const { return dimensions.windowWidth; }
    int getWindowHeight() const { return dimensions.windowHeight; }
    
private:
    CoordinateSystem() = default;
    ~CoordinateSystem() = default;
    CoordinateSystem(const CoordinateSystem&) = delete;
    CoordinateSystem& operator=(const CoordinateSystem&) = delete;
    
    GLFWwindow* window = nullptr;
    WindowDimensions dimensions = {};
    bool initialized = false;
    
    // Saved OpenGL state for restoration
    struct SavedOpenGLState {
        bool depthTest;
        bool blend;
        int blendSrcRGB, blendDstRGB, blendSrcAlpha, blendDstAlpha;
        int blendEquationRGB, blendEquationAlpha;
        float lineWidth;
        int viewport[4];
    } savedState;
    
    void updateDimensions();
};
