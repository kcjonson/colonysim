#include "VectorGraphics.h"
#include "Rendering/Layer.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

bool Shader::loadFromFile(const char* vertexPath, const char* fragmentPath) {
    // Get the executable's directory
    std::filesystem::path exePath = std::filesystem::current_path();
    std::filesystem::path shaderDir = exePath / "shaders";
    
    // Construct full paths
    std::filesystem::path fullVertexPath = shaderDir / vertexPath;
    std::filesystem::path fullFragmentPath = shaderDir / fragmentPath;

    // Read vertex shader
    std::string vertexCode;
    std::ifstream vShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        vShaderFile.open(fullVertexPath);
        std::stringstream vShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        vShaderFile.close();
        vertexCode = vShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::VERTEX::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        std::cerr << "Tried to open: " << fullVertexPath << std::endl;
        return false;
    }

    // Read fragment shader
    std::string fragmentCode;
    std::ifstream fShaderFile;
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        fShaderFile.open(fullFragmentPath);
        std::stringstream fShaderStream;
        fShaderStream << fShaderFile.rdbuf();
        fShaderFile.close();
        fragmentCode = fShaderStream.str();
    } catch (std::ifstream::failure& e) {
        std::cerr << "ERROR::SHADER::FRAGMENT::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
        std::cerr << "Tried to open: " << fullFragmentPath << std::endl;
        return false;
    }

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // Fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // Shader program
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return false;
    }

    // Delete shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return true;
}

void Shader::setUniform(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, &value[0][0]);
}

VectorGraphics::VectorGraphics(FontRenderer& fontRenderer) 
    : initialized(false)
    , fontRenderer(fontRenderer) {
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Color (location = 2)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

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

    if (!vertices.empty()) {
        // Update buffers if needed
        updateBuffers();

        // Actually render
        shader.use();
        shader.setUniform("view", viewMatrix);
        shader.setUniform("projection", projectionMatrix);

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        #ifdef DEBUG_MODE
            GLenum err;
            while ((err = glGetError()) != GL_NO_ERROR) {
                std::cerr << "OpenGL error: " << err << std::endl;
            }
        #endif
    }
    
    // Render all text commands after other geometry has been drawn
    // This allows for proper layer ordering if no z-index is set
    // This might be a problem if we want text under things.
    for (const auto& cmd : textCommands) {
        // Convert RGBA color to RGB for font renderer
        glm::vec3 textColor(cmd.color.r, cmd.color.g, cmd.color.b);
        
        // Adjust the y-position for baseline alignment
        // The FontRenderer's algorithm works with the top-left position,
        // but we need to offset it to get baseline alignment
        glm::vec2 adjustedPosition = cmd.position;
        adjustedPosition.y += 12.0f * 0.3f; // Apply an upward offset based on font size and scale
        
        fontRenderer.renderText(cmd.text, adjustedPosition, 0.3f, textColor);
    }
    
    // Clear the text commands for the next frame
    textCommands.clear();
}

void VectorGraphics::clear() {
    vertices.clear();
    indices.clear();
    textCommands.clear();
    updateBuffers(); // Why is this here?
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
    // Do not clear the vertices and indices here, as we probally have not rendered yet!
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
    // If there's no border, just draw a normal rectangle
    if (borderWidth <= 0.0f) {
        size_t startIndex = vertices.size();
        
        // Add vertices
        vertices.push_back({position + glm::vec2(-size.x/2, -size.y/2), color});
        vertices.push_back({position + glm::vec2(size.x/2, -size.y/2), color});
        vertices.push_back({position + glm::vec2(size.x/2, size.y/2), color});
        vertices.push_back({position + glm::vec2(-size.x/2, size.y/2), color});

        // Add indices
        indices.push_back(static_cast<unsigned int>(startIndex));
        indices.push_back(static_cast<unsigned int>(startIndex + 1));
        indices.push_back(static_cast<unsigned int>(startIndex + 2));
        indices.push_back(static_cast<unsigned int>(startIndex));
        indices.push_back(static_cast<unsigned int>(startIndex + 2));
        indices.push_back(static_cast<unsigned int>(startIndex + 3));

        if (!isBatching) {
            updateBuffers();
        }
        return;
    }

    // Calculate inner and outer rectangles based on border position
    glm::vec2 innerSize = size;
    glm::vec2 outerSize = size;
    
    switch (borderPosition) {
        case BorderPosition::Inside:
            innerSize -= glm::vec2(borderWidth * 2.0f);
            break;
        case BorderPosition::Outside:
            outerSize += glm::vec2(borderWidth * 2.0f);
            break;
        case BorderPosition::Center:
            innerSize -= glm::vec2(borderWidth);
            outerSize += glm::vec2(borderWidth);
            break;
    }

    // Ensure inner rectangle doesn't have negative dimensions
    innerSize = glm::max(innerSize, glm::vec2(0.0f));

    // Draw the outer rectangle with border color
    size_t outerStartIndex = vertices.size();
    
    // Add vertices for outer rectangle
    vertices.push_back({position + glm::vec2(-outerSize.x/2, -outerSize.y/2), borderColor});
    vertices.push_back({position + glm::vec2(outerSize.x/2, -outerSize.y/2), borderColor});
    vertices.push_back({position + glm::vec2(outerSize.x/2, outerSize.y/2), borderColor});
    vertices.push_back({position + glm::vec2(-outerSize.x/2, outerSize.y/2), borderColor});

    // Add indices for outer rectangle
    indices.push_back(static_cast<unsigned int>(outerStartIndex));
    indices.push_back(static_cast<unsigned int>(outerStartIndex + 1));
    indices.push_back(static_cast<unsigned int>(outerStartIndex + 2));
    indices.push_back(static_cast<unsigned int>(outerStartIndex));
    indices.push_back(static_cast<unsigned int>(outerStartIndex + 2));
    indices.push_back(static_cast<unsigned int>(outerStartIndex + 3));

    // Draw the inner rectangle with fill color if it has area
    if (innerSize.x > 0 && innerSize.y > 0) {
        size_t innerStartIndex = vertices.size();
        
        // Add vertices for inner rectangle
        vertices.push_back({position + glm::vec2(-innerSize.x/2, -innerSize.y/2), color});
        vertices.push_back({position + glm::vec2(innerSize.x/2, -innerSize.y/2), color});
        vertices.push_back({position + glm::vec2(innerSize.x/2, innerSize.y/2), color});
        vertices.push_back({position + glm::vec2(-innerSize.x/2, innerSize.y/2), color});

        // Add indices for inner rectangle
        indices.push_back(static_cast<unsigned int>(innerStartIndex));
        indices.push_back(static_cast<unsigned int>(innerStartIndex + 1));
        indices.push_back(static_cast<unsigned int>(innerStartIndex + 2));
        indices.push_back(static_cast<unsigned int>(innerStartIndex));
        indices.push_back(static_cast<unsigned int>(innerStartIndex + 2));
        indices.push_back(static_cast<unsigned int>(innerStartIndex + 3));
    }

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
    // If there's no border, just draw a normal circle
    if (borderWidth <= 0.0f) {
        size_t startIndex = vertices.size();
        
        // Add center vertex
        vertices.push_back({center, color});
        
        // Add vertices around the circle
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            glm::vec2 offset(radius * cos(angle), radius * sin(angle));
            vertices.push_back({center + offset, color});
        }

        // Add indices for triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 1));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 2));
        }

        if (!isBatching) {
            updateBuffers();
        }
        return;
    }

    // Calculate inner and outer radius based on border position
    float innerRadius = radius;
    float outerRadius = radius;
    
    switch (borderPosition) {
        case BorderPosition::Inside:
            innerRadius -= borderWidth;
            break;
        case BorderPosition::Outside:
            outerRadius += borderWidth;
            break;
        case BorderPosition::Center:
            innerRadius -= borderWidth / 2.0f;
            outerRadius += borderWidth / 2.0f;
            break;
    }

    // Ensure inner radius doesn't go negative
    innerRadius = std::max(innerRadius, 0.0f);

    // Draw the outer circle with border color
    size_t outerStartIndex = vertices.size();
    
    // Add center vertex for outer circle
    vertices.push_back({center, borderColor});
    
    // Add vertices around the outer circle
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        glm::vec2 offset(outerRadius * cos(angle), outerRadius * sin(angle));
        vertices.push_back({center + offset, borderColor});
    }

    // Add indices for outer circle triangles
    for (int i = 0; i < segments; ++i) {
        indices.push_back(static_cast<unsigned int>(outerStartIndex));
        indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 1));
        indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 2));
    }

    // Draw the inner circle with fill color if it has area
    if (innerRadius > 0) {
        size_t innerStartIndex = vertices.size();
        
        // Add center vertex for inner circle
        vertices.push_back({center, color});
        
        // Add vertices around the inner circle
        for (int i = 0; i <= segments; ++i) {
            float angle = 2.0f * glm::pi<float>() * i / segments;
            glm::vec2 offset(innerRadius * cos(angle), innerRadius * sin(angle));
            vertices.push_back({center + offset, color});
        }

        // Add indices for inner circle triangles
        for (int i = 0; i < segments; ++i) {
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 1));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 2));
        }
    }

    if (!isBatching) {
        updateBuffers();
    }
}

void VectorGraphics::drawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float width) {
    glm::vec2 direction = glm::normalize(end - start);
    glm::vec2 perpendicular(-direction.y, direction.x);
    glm::vec2 offset = perpendicular * (width / 2.0f);

    size_t startIndex = vertices.size();
    
    // Add vertices
    vertices.push_back({start - offset, color});
    vertices.push_back({start + offset, color});
    vertices.push_back({end + offset, color});
    vertices.push_back({end - offset, color});

    // Add indices
    indices.push_back(static_cast<unsigned int>(startIndex));
    indices.push_back(static_cast<unsigned int>(startIndex + 1));
    indices.push_back(static_cast<unsigned int>(startIndex + 2));
    indices.push_back(static_cast<unsigned int>(startIndex));
    indices.push_back(static_cast<unsigned int>(startIndex + 2));
    indices.push_back(static_cast<unsigned int>(startIndex + 3));

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
    if (points.size() < 3) return;

    // If there's no border, just draw a normal polygon
    if (borderWidth <= 0.0f) {
        size_t startIndex = vertices.size();
        
        // Add vertices
        for (const auto& point : points) {
            vertices.push_back({point, color});
        }

        // Add indices for triangles using triangle fan
        for (size_t i = 1; i < points.size() - 1; ++i) {
            indices.push_back(static_cast<unsigned int>(startIndex));
            indices.push_back(static_cast<unsigned int>(startIndex + i));
            indices.push_back(static_cast<unsigned int>(startIndex + i + 1));
        }

        if (!isBatching) {
            updateBuffers();
        }
        return;
    }

    // For polygons with borders, we'll draw two polygons:
    // 1. The outer polygon with the border color
    // 2. The inner polygon with the fill color (if applicable)

    // For simplicity, we'll just support the BorderPosition::Inside for polygons for now
    // A more complex implementation would need to calculate offset polygons

    // Draw the outer polygon with border color
    size_t outerStartIndex = vertices.size();
    
    // Add vertices for outer polygon
    for (const auto& point : points) {
        vertices.push_back({point, borderColor});
    }

    // Add indices for outer polygon
    for (size_t i = 1; i < points.size() - 1; ++i) {
        indices.push_back(static_cast<unsigned int>(outerStartIndex));
        indices.push_back(static_cast<unsigned int>(outerStartIndex + i));
        indices.push_back(static_cast<unsigned int>(outerStartIndex + i + 1));
    }

    // Calculate and draw inner polygon with fill color
    if (borderWidth > 0) {
        // Calculate the centroid of the polygon
        glm::vec2 centroid(0.0f, 0.0f);
        for (const auto& point : points) {
            centroid += point;
        }
        centroid /= static_cast<float>(points.size());
        
        // Calculate scaled-down inner polygon points
        std::vector<glm::vec2> innerPoints;
        float scaleFactor = std::max(0.0f, 1.0f - (borderWidth / 100.0f)); // Simple approximation
        
        for (const auto& point : points) {
            // Scale each point toward the centroid
            glm::vec2 innerPoint = centroid + scaleFactor * (point - centroid);
            innerPoints.push_back(innerPoint);
        }
        
        // Draw the inner polygon with fill color
        size_t innerStartIndex = vertices.size();
        
        // Add vertices for inner polygon
        for (const auto& point : innerPoints) {
            vertices.push_back({point, color});
        }

        // Add indices for inner polygon
        for (size_t i = 1; i < innerPoints.size() - 1; ++i) {
            indices.push_back(static_cast<unsigned int>(innerStartIndex));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i));
            indices.push_back(static_cast<unsigned int>(innerStartIndex + i + 1));
        }
    }

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

void VectorGraphics::drawText(const std::string& text, const glm::vec2& position, const glm::vec4& color) {
    // Store text rendering commands for later execution
    // This ensures that text is rendered in the proper z-index order
    // as part of the Layer rendering system
    TextCommand cmd;
    cmd.text = text;
    cmd.position = position;
    cmd.color = color;
    textCommands.push_back(cmd);
} 