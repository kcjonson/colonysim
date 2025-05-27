#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

class CoordinateSystem {
public:
    static CoordinateSystem& getInstance() {
        static CoordinateSystem instance;
        return instance;
    }

    glm::mat4 createScreenSpaceProjection() const;
    glm::mat4 createWorldSpaceProjection() const;
    glm::vec2 getWindowSize() const;
    void resetOpenGLState() const;
    bool initialize(GLFWwindow* window);
    void setFullViewport() const;
    void updateWindowSize(int width, int height);

    // Coordinate conversion methods
    // These are used internally by the rendering system to handle high-DPI displays
    float getPixelRatio() const;
    glm::vec2 windowToFramebuffer(const glm::vec2& windowCoords) const;
    glm::vec2 framebufferToWindow(const glm::vec2& fbCoords) const;
    
    // Percentage-based layout helpers
    // These allow UI elements to use relative sizing (e.g., "50%" of screen width)
    float percentWidth(float percent) const;
    float percentHeight(float percent) const;
    glm::vec2 percentSize(float widthPercent, float heightPercent) const;
    glm::vec2 percentPosition(float xPercent, float yPercent) const;

private:
    CoordinateSystem() = default;
    ~CoordinateSystem() = default;
    CoordinateSystem(const CoordinateSystem&) = delete;
    CoordinateSystem& operator=(const CoordinateSystem&) = delete;
    
    GLFWwindow* window = nullptr;
    mutable float cachedPixelRatio = 1.0f;
    mutable bool pixelRatioDirty = true;
};