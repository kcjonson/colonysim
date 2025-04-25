#include "GlobeRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <iostream>

namespace WorldGen {

GlobeRenderer::GlobeRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_ibo(0)
    , m_shaderProgram(0)
    , m_rotationAngle(0.0f)
    , m_cameraDistance(5.0f)
    , m_viewportWidth(800)
    , m_viewportHeight(600) {

    m_planetData = std::make_unique<PlanetData>(1.0f, 64);  // second arg is resolution (increase for more detail, hurts performance)
}

GlobeRenderer::~GlobeRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
}

bool GlobeRenderer::initialize() {
    if (!compileShaders()) {
        return false;
    }
    
    setupBuffers();
    updateModelMatrix();
    
    // Set default light and view positions
    glUseProgram(m_shaderProgram);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "lightPos"), 5.0f, 5.0f, 5.0f);
    glUniform3f(glGetUniformLocation(m_shaderProgram, "viewPos"), 0.0f, 0.0f, m_cameraDistance);
    
    return true;
}

void GlobeRenderer::render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    glUseProgram(m_shaderProgram);
    
    // Update matrices
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, &m_modelMatrix[0][0]);
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    // Update view position
    glm::vec3 viewPos = glm::vec3(0.0f, 0.0f, m_cameraDistance);
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, &viewPos[0]);
    
    // Set light direction (pointing towards the planet)
    glm::vec3 lightDir = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
    glUniform3fv(m_lightDirLoc, 1, &lightDir[0]);
    
    // Set light color (white light)
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(m_lightColorLoc, 1, &lightColor[0]);
    
    // Set planet base color (ocean blue)
    glm::vec3 planetColor = glm::vec3(0.0f, 0.3f, 0.8f);
    glUniform3fv(m_planetColorLoc, 1, &planetColor[0]);
    
    // Draw the planet
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_planetData->getIndices().size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void GlobeRenderer::setRotationAngle(float angle) {
    m_rotationAngle = angle;
    updateModelMatrix();
}

void GlobeRenderer::setCameraDistance(float distance) {
    m_cameraDistance = distance;
}

void GlobeRenderer::resize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

bool GlobeRenderer::compileShaders() {
    // Load shader source code from files
    std::string vertexShaderPath = "shaders/Planet/PlanetVertex.glsl";
    std::string fragmentShaderPath = "shaders/Planet/PlanetFragment.glsl";
    
    std::ifstream vertexShaderFile(vertexShaderPath);
    std::ifstream fragmentShaderFile(fragmentShaderPath);
    
    if (!vertexShaderFile.is_open() || !fragmentShaderFile.is_open()) {
        std::cerr << "Failed to open shader files" << std::endl;
        return false;
    }
    
    std::stringstream vertexStream, fragmentStream;
    vertexStream << vertexShaderFile.rdbuf();
    fragmentStream << fragmentShaderFile.rdbuf();
    
    std::string vertexShaderSource = vertexStream.str();
    std::string fragmentShaderSource = fragmentStream.str();
    
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexSource = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
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
    const char* fragmentSource = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
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
    m_lightDirLoc = glGetUniformLocation(m_shaderProgram, "lightDir");
    m_lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    m_planetColorLoc = glGetUniformLocation(m_shaderProgram, "planetColor");
    
    return true;
}

void GlobeRenderer::setupBuffers() {
    // Create and bind VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);
    
    // Create and bind VBO
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // Combine vertex data (position, texcoord, normal)
    const auto& vertices = m_planetData->getVertices();
    const auto& texCoords = m_planetData->getTexCoords();
    const auto& normals = m_planetData->getNormals();
    
    std::vector<float> combinedData;
    combinedData.reserve(vertices.size() + texCoords.size() + normals.size());
    
    for (size_t i = 0; i < vertices.size() / 3; i++) {
        // Position
        combinedData.push_back(vertices[i * 3]);
        combinedData.push_back(vertices[i * 3 + 1]);
        combinedData.push_back(vertices[i * 3 + 2]);
        
        // Texture coordinates
        combinedData.push_back(texCoords[i * 2]);
        combinedData.push_back(texCoords[i * 2 + 1]);
        
        // Normal
        combinedData.push_back(normals[i * 3]);
        combinedData.push_back(normals[i * 3 + 1]);
        combinedData.push_back(normals[i * 3 + 2]);
    }
    
    glBufferData(GL_ARRAY_BUFFER, combinedData.size() * sizeof(float), combinedData.data(), GL_STATIC_DRAW);
    
    // Set up vertex attributes
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinates
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Create and bind IBO
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_planetData->getIndices().size() * sizeof(unsigned int),
                 m_planetData->getIndices().data(), GL_STATIC_DRAW);
    
    // Unbind VAO
    glBindVertexArray(0);
}

void GlobeRenderer::updateModelMatrix() {
    m_modelMatrix = glm::mat4(1.0f);
    m_modelMatrix = glm::rotate(m_modelMatrix, m_rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
}

} // namespace WorldGen