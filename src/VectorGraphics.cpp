#include "VectorGraphics.h"
#include "Rendering/Layer.h"
#include "Renderer.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

VectorGraphics::VectorGraphics() 
    : initialized(false)
    , renderer(nullptr) {
    // Initialize member variables
    VAO = 0;
    VBO = 0;
    EBO = 0;
}

VectorGraphics::~VectorGraphics() {
    if (initialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
}

bool VectorGraphics::initialize() {
    if (initialized) {
        std::cout << "VectorGraphics already initialized" << std::endl;
        return true;
    }

    std::cout << "Initializing VectorGraphics..." << std::endl;

    // Initialize shader
    if (!shader.loadFromFile("vector.vert", "vector.frag")) {
        std::cerr << "Failed to load vector shaders" << std::endl;
        return false;
    }
    std::cout << "Shader loaded successfully" << std::endl;

    // Initialize buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Set up vertex attributes
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // Position (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Color (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    initialized = true;
    std::cout << "VectorGraphics initialization complete" << std::endl;
    return true;
}

void VectorGraphics::render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!initialized) {
        std::cerr << "VectorGraphics not initialized" << std::endl;
        return;
    }

    static int renderCallCount = 0;
    renderCallCount++;
    
    if (renderCallCount < 5) {
        std::cout << "VectorGraphics::render() call " << renderCallCount << " with " << vertices.size() << " vertices, " << indices.size() << " indices" << std::endl;
    }

    if (!vertices.empty()) {
        // Update buffers if needed
        updateBuffers();

        // Accumulate frame statistics *before* drawing
        frameVertices += vertices.size();
        frameIndices += indices.size();

        // Actually render
        shader.use();
        shader.setUniform("view", viewMatrix);
        shader.setUniform("projection", projectionMatrix);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // Render all text commands using the Renderer
    if (renderer != nullptr) {
        // Save current OpenGL scissor state
        GLboolean wasScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        GLint originalScissorBox[4];
        if (wasScissorEnabled) {
            glGetIntegerv(GL_SCISSOR_BOX, originalScissorBox);
        }

        // Set the view and projection matrices on the renderer
        renderer->setView(viewMatrix);
        renderer->setProjection(projectionMatrix);
        
        for (const auto& cmd : textCommands) {
            // Convert RGBA color to RGB for text renderer
            glm::vec3 textColor(cmd.color.r, cmd.color.g, cmd.color.b);

            // Apply scissor if this command has one
            if (cmd.scissorBox.has_value()) {
                glEnable(GL_SCISSOR_TEST);
                const auto& box = cmd.scissorBox.value();
                glScissor(box.x, box.y, box.z, box.w);
            } else if (wasScissorEnabled) {
                // If no specific scissor for this command but there was one active,
                // temporarily disable it
                glDisable(GL_SCISSOR_TEST);
            }
            
            // Use the Renderer to render text
            renderer->renderText(cmd.text, cmd.position, cmd.size, textColor);
        }
        
        // Restore original scissor state
        if (wasScissorEnabled) {
            glEnable(GL_SCISSOR_TEST);
            glScissor(originalScissorBox[0], originalScissorBox[1], 
                     originalScissorBox[2], originalScissorBox[3]);
        } else {
            glDisable(GL_SCISSOR_TEST);
        }
        
        textCommands.clear();
    } else {
        std::cerr << "Warning: Text rendering attempted with null renderer" << std::endl;
    }
}

void VectorGraphics::clear() {
    vertices.clear();
    indices.clear();
    textCommands.clear(); // Clear text commands when explicitly asked to clear everything
    updateBuffers();
}

void VectorGraphics::beginFrame() {
    frameVertices = 0;
    frameIndices = 0;
}

void VectorGraphics::beginBatch() {
    if (!isBatching) {
        vertices.clear();
        indices.clear();
        textCommands.clear();
        isBatching = true;
    }
}

void VectorGraphics::endBatch() {
    isBatching = false;
    // We'll clear text commands here after rendering the batch 
    // This ensures text commands are preserved until they're actually rendered
    
    // Note: We're not clearing vertices and indices here since they may be 
    // used later by the render method
}

void VectorGraphics::drawRectangle(
    const glm::vec2& position, 
    const glm::vec2& size, 
    const glm::vec4& color,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition,
    float cornerRadius
) {
    // Delegate to the Rectangle drawing class
    Rendering::Draw::Rectangle::draw(
        position,
        size,
        color,
        vertices,
        indices,
        borderColor,
        borderWidth,
        borderPosition,
        cornerRadius
    );

    if (!isBatching) {
        updateBuffers();
    }
}

void VectorGraphics::drawCircle(
    const glm::vec2& center, 
    float radius, 
    const glm::vec4& color,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition,
    int segments
) {
    // Delegate to the Circle drawing class
    Rendering::Draw::Circle::draw(
        center,
        radius,
        color,
        vertices,
        indices,
        borderColor,
        borderWidth,
        borderPosition,
        segments
    );

    if (!isBatching) {
        updateBuffers();
    }
}

void VectorGraphics::drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float width) {
    // Delegate to the Line drawing class
    Rendering::Draw::Line::draw(
        start,
        end,
        color,
        vertices,
        indices,
        width
    );

    if (!isBatching) {
        updateBuffers();
    }
}

void VectorGraphics::drawPolygon(
    const std::vector<glm::vec2>& points, 
    const glm::vec4& color,
    const glm::vec4& borderColor,
    float borderWidth,
    BorderPosition borderPosition
) {
    // Delegate to the Polygon drawing class
    Rendering::Draw::Polygon::draw(
        points,
        color,
        vertices,
        indices,
        borderColor,
        borderWidth,
        borderPosition
    );

    if (!isBatching) {
        updateBuffers();
    }
}

void VectorGraphics::updateBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_DRAW);
}

void VectorGraphics::drawText(const std::string& text, const glm::vec2& position, const glm::vec4& color, float size) {
    // Store text rendering commands for later execution by the Renderer
    TextCommand cmd;
    cmd.text = text;
    cmd.position = position;
    cmd.color = color;
    cmd.size = size;
    cmd.scissorBox = currentScissorBox;
    textCommands.push_back(cmd);
}

void VectorGraphics::setScissor(int x, int y, int width, int height) {
    currentScissorBox = glm::ivec4(x, y, width, height);
}

void VectorGraphics::clearScissor() {
    currentScissorBox.reset();
}

glm::vec2 VectorGraphics::measureText(const std::string& text, float size) const {
    // Delegate to the renderer, which will call FontRenderer's measureText
    if (renderer) {
        return renderer->measureText(text, size);
    }
    
    // Fallback for when renderer is not available
    return glm::vec2(0.0f);
}