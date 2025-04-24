#include "PlateRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <algorithm> // For std::min/max

namespace WorldGen {

PlateRenderer::PlateRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_shaderProgram(0)
    , m_modelLoc(0)
    , m_viewLoc(0)
    , m_projectionLoc(0)
    , m_colorLoc(0)
{
}

PlateRenderer::~PlateRenderer() {
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
}

bool PlateRenderer::initialize() {
    if (!compileShaders()) {
        return false;
    }
    
    setupBuffers();
    return true;
}

void PlateRenderer::render(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices, // Added
    const glm::mat4& modelMatrix,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix)
{
    if (planetVertices.empty()) return; // Cannot render edges without vertices

    glUseProgram(m_shaderProgram);

    // Set matrices
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // Enable line smoothing for better visibility
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(2.0f); // Make lines thicker

    // Render plate boundaries by drawing each edge individually
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo); // Bind the VBO once

    for (const auto& plate : plates) {
        for (const auto& boundary : plate->GetBoundaries()) {
            // Skip if we've already rendered this boundary (convention: plate1Index < plate2Index)
            // Ensure we process each boundary pair only once
            int plate1Id = plate->GetId();
            int plate2Id = (boundary.plate1Index == plate1Id) ? boundary.plate2Index : boundary.plate1Index;
            if (plate1Id >= plate2Id) {
                continue;
            }

            // Set color based on boundary type
            glm::vec3 color;
            switch (boundary.type) {
                case BoundaryType::Convergent:
                    color = glm::vec3(1.0f, 0.0f, 0.0f); // Red
                    break;
                case BoundaryType::Divergent:
                    color = glm::vec3(0.0f, 0.0f, 1.0f); // Blue
                    break;
                case BoundaryType::Transform:
                    color = glm::vec3(0.0f, 1.0f, 0.0f); // Green
                    break;
                default:
                    color = glm::vec3(1.0f, 1.0f, 1.0f); // White (fallback)
            }
            // Optional: Modulate color by stress (e.g., make brighter for higher stress)
            // float stressNormalized = glm::clamp(boundary.stress / 10.0f, 0.0f, 1.0f); // Example normalization
            // color = glm::mix(glm::vec3(0.5f), color, stressNormalized); // Mix with gray based on stress

            glUniform3fv(m_colorLoc, 1, glm::value_ptr(color));

            // Draw each edge segment of the boundary
            for (const auto& edgeIndices : boundary.m_sharedEdgeIndices) {
                int u_idx = edgeIndices.first;
                int v_idx = edgeIndices.second;

                // Ensure indices are valid
                if (u_idx >= 0 && u_idx < planetVertices.size() && v_idx >= 0 && v_idx < planetVertices.size()) {
                    glm::vec3 edgeVertices[2] = {
                        planetVertices[u_idx],
                        planetVertices[v_idx]
                    };

                    // Update VBO data for this single edge
                    // Use glBufferSubData for potentially better performance than glBufferData
                    // Allocate buffer size once if possible, or ensure it's large enough
                    glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec3), edgeVertices, GL_DYNAMIC_DRAW); // Keep using glBufferData for simplicity now

                    // Draw the line segment
                    glDrawArrays(GL_LINES, 0, 2);
                } else {
                    std::cerr << "Warning: Invalid edge index in PlateRenderer." << std::endl;
                }
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Unbind VBO
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f); // Reset line width
    glBindVertexArray(0);
    glUseProgram(0);
}

void PlateRenderer::resize(int width, int height) {
    // No special handling needed for resize
}

bool PlateRenderer::compileShaders() {
    // Load shader source code
    std::string vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    
    std::string fragmentSource = R"(
        #version 330 core
        out vec4 FragColor;
        uniform vec3 color;
        void main() {
            FragColor = vec4(color, 1.0);
        }
    )";
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexSourcePtr = vertexSource.c_str();
    glShaderSource(vertexShader, 1, &vertexSourcePtr, nullptr);
    glCompileShader(vertexShader);
    
    // Check vertex shader compilation
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentSourcePtr = fragmentSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentSourcePtr, nullptr);
    glCompileShader(fragmentShader);
    
    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        return false;
    }
    
    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    // Check program linking
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        return false;
    }
    
    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Get uniform locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    m_colorLoc = glGetUniformLocation(m_shaderProgram, "color");

    // Check if uniforms were found
    if (m_modelLoc == -1 || m_viewLoc == -1 || m_projectionLoc == -1 || m_colorLoc == -1) {
        std::cerr << "Error: Could not get uniform locations in PlateRenderer shader." << std::endl;
        // You might want to delete the program here if locations are critical
        // glDeleteProgram(m_shaderProgram);
        // m_shaderProgram = 0;
        // return false; // Optionally return false if uniforms are essential
    }
    
    return true;
}

void PlateRenderer::setupBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace WorldGen