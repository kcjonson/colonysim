#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include "Vertex.h"
#include "Shader.h"
#include "Rendering/Styles/BorderPosition.h"

// Include the new drawing primitives
#include "Rendering/Draw/Rectangle.h"
#include "Rendering/Draw/Circle.h"
#include "Rendering/Draw/Line.h"
#include "Rendering/Draw/Polygon.h"

class Renderer; // Forward declaration

// For storing text rendering commands to be executed in order
struct TextCommand {
    std::string text;
    glm::vec2 position;
    glm::vec4 color;
    float size;
};

class VectorGraphics {
public:
    // Delete copy/move constructors and assignment operators
    VectorGraphics(const VectorGraphics&) = delete;
    VectorGraphics& operator=(const VectorGraphics&) = delete;
    VectorGraphics(VectorGraphics&&) = delete;
    VectorGraphics& operator=(VectorGraphics&&) = delete;

    // Static method to access the singleton instance
    static VectorGraphics& getInstance() {
        static VectorGraphics instance;
        return instance;
    }

    // Set the renderer to use for text rendering
    void setRenderer(Renderer* rendererPtr) { renderer = rendererPtr; }

    // Initialize OpenGL resources after GLAD is initialized
    bool initialize();
    
    // Rendering methods
    void render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);  // Render the current batch
    void clear();                                                                // Clear all vertices and indices
    void beginBatch();                                                           // Start a new batch of drawing operations
    void endBatch();                                                             // End the current batch and prepare for rendering
    bool isBatching = false;                                                     // Whether currently batching draw calls

    void beginFrame(); // Add this method to reset frame counters

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
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     * @param cornerRadius Radius of rounded corners (0 for sharp corners)
     */
    void drawRectangle(
        const glm::vec2& position, 
        const glm::vec2& size, 
        const glm::vec4& color,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside,
        float cornerRadius = 0.0f
    );

    /**
     * Draw a circle centered at the given position
     * @param center Center point of the circle
     * @param radius Radius of the circle
     * @param color RGBA color (0-1 range)
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     * @param segments Number of segments to approximate the circle (default: 32)
     */
    void drawCircle(
        const glm::vec2& center, 
        float radius, 
        const glm::vec4& color,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside,
        int segments = 32
    );

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
     * @param borderColor Border color (if borderWidth > 0)
     * @param borderWidth Width of the border (0 for no border)
     * @param borderPosition Position of the border (inside, outside, center)
     */
    void drawPolygon(
        const std::vector<glm::vec2>& points, 
        const glm::vec4& color,
        const glm::vec4& borderColor = glm::vec4(0.0f),
        float borderWidth = 0.0f,
        BorderPosition borderPosition = BorderPosition::Outside
    );
      /**
     * Draw text using the Renderer
     * @param text String to render
     * @param position Top-left position of the text
     * @param color RGBA color (0-1 range)
     * @param size Size of the text (default: 0.3f)
     */    void drawText(const std::string& text, const glm::vec2& position, const glm::vec4& color, float size = 0.3f);
      /**
     * Measure text dimensions
     * @param text String to measure
     * @param size Size factor for the text (default: 0.3f)
     * @return Width and height of the text in screen units
     */
    glm::vec2 measureText(const std::string& text, float size = 0.3f) const;

    // Get rendering statistics
    size_t getTotalVertices() const { return vertices.size(); } // Current buffer size
    size_t getTotalIndices() const { return indices.size(); }   // Current buffer size
    size_t getFrameVertices() const { return frameVertices; } // Total vertices rendered in the frame
    size_t getFrameIndices() const { return frameIndices; }   // Total indices rendered in the frame

private:
    // Private constructor for singleton
    VectorGraphics();
    ~VectorGraphics();

    void updateBuffers();  // Update OpenGL buffers with current vertices and indices

    Shader shader;
    std::vector<Vertex> vertices;      // List of vertices to render
    std::vector<unsigned int> indices; // List of indices defining triangles

    // Frame statistics
    size_t frameVertices = 0;
    size_t frameIndices = 0;

    GLuint VAO;                        // Vertex Array Object
    GLuint VBO;                        // Vertex Buffer Object
    GLuint EBO;                        // Element Buffer Object
    bool initialized;                  // Whether the graphics system is initialized
    Renderer* renderer;                // Pointer to the renderer
    std::vector<TextCommand> textCommands; // List of text commands to execute in order
};