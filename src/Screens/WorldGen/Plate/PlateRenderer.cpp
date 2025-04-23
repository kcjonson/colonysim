#include "PlateRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>

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
    const glm::mat4& modelMatrix,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix)
{
    glUseProgram(m_shaderProgram);
    
    // Set matrices
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    // Enable line smoothing for better visibility
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Render plate boundaries
    glBindVertexArray(m_vao);
    
    for (const auto& plate : plates) {
        for (const auto& boundary : plate->GetBoundaries()) {
            // Skip if we've already rendered this boundary
            if (boundary.plate1Index > boundary.plate2Index) {
                continue;
            }
            
            // Set color based on stress
            float stress = boundary.stress;
            // Use a more visible color scheme: red for high stress, blue for low stress
            glm::vec3 color(stress, 0.2f, 1.0f - stress);
            glUniform3fv(m_colorLoc, 1, &color[0]);
            
            // Update boundary buffer with the current boundary's points
            std::vector<glm::vec3> boundaryPoints = boundary.points;
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, boundaryPoints.size() * sizeof(glm::vec3),
                         boundaryPoints.data(), GL_DYNAMIC_DRAW);
            
            // Draw boundary
            glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(boundaryPoints.size()));
        }
    }
    
    glDisable(GL_LINE_SMOOTH);
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

void PlateRenderer::updateBoundaryBuffers(const std::vector<std::shared_ptr<TectonicPlate>>& plates) {
    std::vector<glm::vec3> boundaryPoints;
    
    for (const auto& plate : plates) {
        for (const auto& boundary : plate->GetBoundaries()) {
            if (boundary.plate1Index > boundary.plate2Index) {
                continue;
            }
            boundaryPoints.insert(boundaryPoints.end(), boundary.points.begin(), boundary.points.end());
        }
    }
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, boundaryPoints.size() * sizeof(glm::vec3),
                 boundaryPoints.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

} // namespace WorldGen 