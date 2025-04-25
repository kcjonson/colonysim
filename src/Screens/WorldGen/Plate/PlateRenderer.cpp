#include "PlateRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <algorithm> // For std::min/max
#include <vector> // Ensure vector is included

namespace WorldGen {

PlateRenderer::PlateRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_colorVbo(0) // Initialize color VBO
    , m_shaderProgram(0)
    , m_modelLoc(0)
    , m_viewLoc(0)
    , m_projectionLoc(0)
{
}

PlateRenderer::~PlateRenderer() {
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
    }
    if (m_colorVbo) {
        glDeleteBuffers(1, &m_colorVbo);
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
    if (planetVertices.empty() || plates.empty()) return;

    std::vector<glm::vec3> lineVertices;
    std::vector<glm::vec3> lineColors;

    // Estimate capacity to reduce reallocations (rough estimate)
    size_t estimatedEdges = 0;
    for (const auto& plate : plates) {
        for (const auto& boundary : plate->GetBoundaries()) {
            estimatedEdges += boundary.m_sharedEdgeIndices.size();
        }
    }
    lineVertices.reserve(estimatedEdges * 2);
    lineColors.reserve(estimatedEdges * 2);

    // Collect all line vertices and colors
    for (const auto& plate : plates) {
        for (const auto& boundary : plate->GetBoundaries()) {
            // Ensure we process each boundary pair only once
            int plate1Id = plate->GetId();
            int plate2Id = (boundary.plate1Index == plate1Id) ? boundary.plate2Index : boundary.plate1Index;
            if (plate1Id >= plate2Id) {
                continue;
            }

            // Determine color based on boundary type
            glm::vec3 color;
            switch (boundary.type) {
                case BoundaryType::Convergent:  color = glm::vec3(1.0f, 0.0f, 0.0f); break; // Red
                case BoundaryType::Divergent:   color = glm::vec3(0.0f, 0.0f, 1.0f); break; // Blue
                case BoundaryType::Transform:   color = glm::vec3(0.0f, 1.0f, 0.0f); break; // Green
                default:                        color = glm::vec3(1.0f, 1.0f, 1.0f); break; // White
            }

            // Add vertices and colors for each edge segment
            for (const auto& edgeIndices : boundary.m_sharedEdgeIndices) {
                int u_idx = edgeIndices.first;
                int v_idx = edgeIndices.second;

                if (u_idx >= 0 && u_idx < planetVertices.size() && v_idx >= 0 && v_idx < planetVertices.size()) {
                    lineVertices.push_back(planetVertices[u_idx]);
                    lineVertices.push_back(planetVertices[v_idx]);
                    lineColors.push_back(color);
                    lineColors.push_back(color);
                } else {
                    std::cerr << "Warning: Invalid edge index in PlateRenderer." << std::endl;
                }
            }
        }
    }

    if (lineVertices.empty()) return; // Nothing to draw

    glUseProgram(m_shaderProgram);

    // Set matrices
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));

    // Enable line smoothing and set width
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(2.0f);

    // Bind VAO
    glBindVertexArray(m_vao);

    // Update vertex position buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), lineVertices.data(), GL_DYNAMIC_DRAW);

    // Update vertex color buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
    glBufferData(GL_ARRAY_BUFFER, lineColors.size() * sizeof(glm::vec3), lineColors.data(), GL_DYNAMIC_DRAW);

    // Draw all lines at once
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size()));

    // Unbind and cleanup state
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
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
        layout (location = 1) in vec3 aColor; // Added color attribute

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 vColor; // Pass color to fragment shader

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    std::string fragmentSource = R"(
        #version 330 core
        in vec3 vColor; // Receive color from vertex shader
        out vec4 FragColor;

        void main() {
            FragColor = vec4(vColor, 1.0);
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

    // Check if uniforms were found
    if (m_modelLoc == -1 || m_viewLoc == -1 || m_projectionLoc == -1) {
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
    glGenBuffers(1, &m_vbo);      // For positions
    glGenBuffers(1, &m_colorVbo); // For colors

    glBindVertexArray(m_vao);

    // Position attribute (location 0)
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    // We'll upload data in render(), so just configure the attribute pointer for now
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location 1)
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
    // We'll upload data in render(), so just configure the attribute pointer for now
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace WorldGen