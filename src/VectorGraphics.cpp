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

VectorGraphics::VectorGraphics() : initialized(false) {
    // Initialize member variables
    VAO = 0;
    VBO = 0;
    EBO = 0;
    layers.clear();
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

    // First sort layers by z-index
    sortLayers();

    // Begin batching for all layers
    beginBatch();

    // Render all layers
    std::cout << "Rendering " << layers.size() << " layers" << std::endl;
    for (auto& layer : layers) {
        layer->render(*this, viewMatrix, projectionMatrix);
    }

    // End batching for all layers and update buffers
    endBatch();

    // Actually render
    std::cout << "Drawing " << indices.size() << " indices" << std::endl;
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

void VectorGraphics::clear() {
    vertices.clear();
    indices.clear();
    updateBuffers();
}

void VectorGraphics::beginBatch() {
    vertices.clear();
    indices.clear();
    isBatching = true;
}

void VectorGraphics::endBatch() {
    if (!vertices.empty()) {
        updateBuffers();
    }
    isBatching = false;
}

void VectorGraphics::drawRectangle(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color) {
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
}

void VectorGraphics::drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments) {
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

void VectorGraphics::drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color) {
    if (points.size() < 3) return;

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
}

void VectorGraphics::updateBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_DYNAMIC_DRAW);
}

void VectorGraphics::drawText(const std::string& text, const glm::vec2& position, const glm::vec4& color) {
    // For now, we'll implement a simple text rendering using rectangles for each character
    // This is a placeholder until we implement proper font rendering
    const float charWidth = 8.0f;
    const float charHeight = 12.0f;
    const float spacing = 2.0f;
    
    glm::vec2 currentPos = position;
    
    for (char c : text) {
        if (c == ' ') {
            currentPos.x += charWidth + spacing;
            continue;
        }
        
        // Draw a simple rectangle for each character
        drawRectangle(currentPos, glm::vec2(charWidth, charHeight), color);
        currentPos.x += charWidth + spacing;
    }
}

void VectorGraphics::renderText(const std::string& text, const glm::vec2& position, const glm::vec4& color) {
    // This is a placeholder for future proper font rendering implementation
    // For now, we'll just call drawText
    drawText(text, position, color);
}

void VectorGraphics::renderScreenSpace(const glm::mat4& projectionMatrix) {
    if (!initialized) {
        std::cerr << "VectorGraphics not initialized" << std::endl;
        return;
    }

    // First sort layers by z-index
    sortLayers();

    // Begin batching for all layers
    beginBatch();

    // Render all layers
    for (auto& layer : layers) {
        layer->renderScreenSpace(*this, projectionMatrix);
    }

    // End batching for all layers
    endBatch();

    // Actually render
    shader.use();
    shader.setUniform("view", glm::mat4(1.0f)); // Identity matrix for screen space
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

void VectorGraphics::addLayer(std::shared_ptr<Rendering::Layer> layer) {
    layers.push_back(layer);
    sortLayers();
}

void VectorGraphics::removeLayer(std::shared_ptr<Rendering::Layer> layer) {
    layers.erase(
        std::remove_if(layers.begin(), layers.end(),
            [&layer](const std::shared_ptr<Rendering::Layer>& l) {
                return l == layer;
            }
        ),
        layers.end()
    );
}

void VectorGraphics::clearLayers() {
    layers.clear();
}

void VectorGraphics::sortLayers() {
    std::sort(layers.begin(), layers.end(),
        [](const std::shared_ptr<Rendering::Layer>& a, const std::shared_ptr<Rendering::Layer>& b) {
            return a->getZIndex() < b->getZIndex();
        }
    );
} 