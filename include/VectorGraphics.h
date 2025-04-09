#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <memory>
#include <map>

struct Vertex {
    glm::vec2 position;  // Position in world/screen coordinates
    glm::vec4 color;     // RGBA color (0-1 range)
};

class Shader {
public:
    Shader() : program(0) {}
    ~Shader() { if (program) glDeleteProgram(program); }
    
    bool loadFromFile(const char* vertexPath, const char* fragmentPath);
    void use() const { glUseProgram(program); }
    void setUniform(const char* name, const glm::mat4& value) const;
    
private:
    GLuint program;
};

// Forward declare Layer for use in VectorGraphics
namespace Rendering {
    class Layer;
}

class VectorGraphics {
public:
    VectorGraphics();
    ~VectorGraphics();

    // Initialize OpenGL resources after GLAD is initialized
    bool initialize();

    // Layer management
    void addLayer(std::shared_ptr<Rendering::Layer> layer);
    void removeLayer(std::shared_ptr<Rendering::Layer> layer);
    void clearLayers();
    
    // Rendering methods
    void render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);  // Render in world space
    void renderScreenSpace(const glm::mat4& projectionMatrix);                    // Render in screen space
    void clear();                                                                 // Clear all vertices and indices
    void beginBatch();                                                            // Start a new batch of drawing operations
    void endBatch();                                                              // End the current batch and render
    bool isBatching = false;                                                      // Whether currently batching draw calls

    /**
     * Drawing primitives
     * All positions are in world/screen coordinates
     * All sizes are in world/screen units
     * All colors are RGBA (0-1 range)
     */

    /**
     * Draw a rectangle centered at the given position
     * @param position Center point of the rectangle
     * @param size Width and height of the rectangle
     * @param color RGBA color (0-1 range)
     */
    void drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

    /**
     * Draw a circle centered at the given position
     * @param center Center point of the circle
     * @param radius Radius of the circle
     * @param color RGBA color (0-1 range)
     * @param segments Number of segments to approximate the circle (default: 32)
     */
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 32);

    /**
     * Draw a line between two points
     * @param start Starting point of the line
     * @param end Ending point of the line
     * @param color RGBA color (0-1 range)
     * @param width Width of the line (default: 1.0f)
     */
    void drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float width = 1.0f);

    /**
     * Draw a polygon defined by a list of points
     * @param points List of points defining the polygon vertices
     * @param color RGBA color (0-1 range)
     */
    void drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color);
    
    /**
     * Draw text using simple rectangles for each character
     * @param text String to render
     * @param position Top-left position of the text
     * @param color RGBA color (0-1 range)
     */
    void drawText(const std::string& text, const glm::vec2& position, const glm::vec4& color);

private:
    void updateBuffers();  // Update OpenGL buffers with current vertices and indices
    void renderText(const std::string& text, const glm::vec2& position, const glm::vec4& color);  // Internal text rendering method
    void sortLayers();     // Sort layers by z-index

    Shader shader;
    std::vector<Vertex> vertices;      // List of vertices to render
    std::vector<unsigned int> indices; // List of indices defining triangles
    GLuint VAO;                        // Vertex Array Object
    GLuint VBO;                        // Vertex Buffer Object
    GLuint EBO;                        // Element Buffer Object
    bool initialized;                  // Whether the graphics system is initialized
    
    std::vector<std::shared_ptr<Rendering::Layer>> layers; // List of layers
}; 