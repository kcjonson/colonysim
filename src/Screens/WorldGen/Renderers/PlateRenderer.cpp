#include "PlateRenderer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr
#include <algorithm> // For std::min/max
#include <vector> // Ensure vector is included
#include <unordered_set> // For unique boundary vertices
#include <chrono> // For perf logging
#include <random> // For unique color generation

namespace WorldGen {

// Helper: Generate a unique color for each plate (HSV to RGB)
static glm::vec3 PlateIdToColor(int plateId, int totalPlates) {
    float hue = (float)plateId / std::max(1, totalPlates); // [0,1)
    float s = 0.7f, v = 0.7f;
    float r, g, b;
    int i = int(hue * 6.0f);
    float f = hue * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    return glm::vec3(r, g, b);
}

PlateRenderer::PlateRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_colorVbo(0) // Initialize color VBO
    , m_shaderProgram(0)
    , m_modelLoc(0)
    , m_viewLoc(0)
    , m_projectionLoc(0)
    , m_thicknessCacheDirty(true) // Initialize cache dirty flag
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

void PlateRenderer::updateThicknessLineCache(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices)
{
    auto start = std::chrono::high_resolution_clock::now();
    m_thicknessLineVertices.clear();
    m_thicknessLineColors.clear();
    std::unordered_set<int> uniqueIndices;
    size_t numLines = 0;

    // Disable thickness lines by not adding any vertices
    // Original thickness line generation code is commented out
    /*
    std::vector<glm::vec4> thicknessLineColors;
    for (const auto& plate : plates) {
        glm::vec3 plateColor = (plate->GetType() == PlateType::Continental) ? glm::vec3(0.8f, 0.7f, 0.3f) : glm::vec3(0.2f, 0.4f, 0.8f);
        glm::vec4 plateColor4 = glm::vec4(plateColor, 1.0f);
        for (int vertexIdx : plate->GetVertexIndices()) {
            if (vertexIdx < 0 || vertexIdx >= planetVertices.size()) continue;
            if (!uniqueIndices.insert(vertexIdx).second) continue; // Only one line per unique vertex
            glm::vec3 pos = planetVertices[vertexIdx];
            float thickness = plate->GetVertexCrustThickness(vertexIdx);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec3 tip = pos + normal * thickness * 0.15f; // Scale for visual clarity
            m_thicknessLineVertices.push_back(pos);
            m_thicknessLineVertices.push_back(tip);
            thicknessLineColors.push_back(plateColor4);
            thicknessLineColors.push_back(plateColor4);
            ++numLines;
        }
    }
    m_thicknessLineColors = thicknessLineColors;
    */
    
    m_thicknessCacheDirty = false;
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[PlateRenderer] Thickness line rendering disabled" << std::endl;
}

void PlateRenderer::render(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices,
    const glm::mat4& modelMatrix,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix)
{
    if (planetVertices.empty() || plates.empty()) return;

    // Save current OpenGL states
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLint currentLineWidth;
    glGetIntegerv(GL_LINE_WIDTH, &currentLineWidth);
    
    // --- Draw boundaries between plates ---
    // Set OpenGL state for boundary line rendering
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(2.0f);
    
    // Use shader and set uniforms
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
    glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
    glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
    
    // Create and draw boundary lines between plates
    glBindVertexArray(m_vao);
    
    // For each plate, render its boundaries
    for (const auto& plate : plates) {
        const auto& boundaries = plate->GetBoundaries();
        
        // Scan through all boundaries of this plate
        for (const auto& boundary : boundaries) {
            // Set color based on boundary type
            glm::vec4 lineColor;
            switch (boundary.type) {
                case BoundaryType::Convergent:
                    lineColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.8f); // Red for convergent
                    break;
                case BoundaryType::Divergent:
                    lineColor = glm::vec4(0.0f, 0.8f, 0.0f, 0.8f); // Green for divergent
                    break;
                case BoundaryType::Transform:
                    lineColor = glm::vec4(0.8f, 0.8f, 0.0f, 0.8f); // Yellow for transform
                    break;
                default:
                    lineColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f); // White for undefined
                    break;
            }
            
            // For each shared vertex in this boundary, we'll draw a line
            // connecting adjacent vertices to form a continuous boundary
            const auto& sharedVertices = boundary.m_sharedVertexIndices;
            if (sharedVertices.size() >= 2) {
                // Create line segments joining adjacent vertices
                std::vector<glm::vec3> lineVertices;
                std::vector<glm::vec4> lineColors;
                
                for (size_t i = 0; i < sharedVertices.size(); i++) {
                    int vertexIndex = sharedVertices[i];
                    if (vertexIndex >= 0 && vertexIndex < planetVertices.size()) {
                        // Small offset to avoid z-fighting
                        glm::vec3 pos = planetVertices[vertexIndex] * 1.001f;
                        lineVertices.push_back(pos);
                        lineColors.push_back(lineColor);
                    }
                }
                
                // Draw the line segments
                if (!lineVertices.empty()) {
                    // Buffer the vertex data
                    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                    glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(glm::vec3), 
                                lineVertices.data(), GL_DYNAMIC_DRAW);
                    
                    // Buffer the color data
                    glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
                    glBufferData(GL_ARRAY_BUFFER, lineColors.size() * sizeof(glm::vec4), 
                                lineColors.data(), GL_DYNAMIC_DRAW);
                    
                    // Draw the lines
                    glBindVertexArray(m_vao);
                    glDrawArrays(GL_LINE_STRIP, 0, static_cast<GLsizei>(lineVertices.size()));
                }
            }
        }
    }
    
    // Unbind VAO and shader
    glBindVertexArray(0);
    glUseProgram(0);
    
    // Also draw thickness lines if cache is dirty
    if (m_thicknessCacheDirty) {
        updateThicknessLineCache(plates, planetVertices);
    }
    
    // Restore previous OpenGL states
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
        
    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
        
    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
        
    glLineWidth(static_cast<GLfloat>(currentLineWidth));
}

void PlateRenderer::resize(int width, int height) {
    // No special handling needed for resize
}

bool PlateRenderer::compileShaders() {
    // Load shader source code
    std::string vertexSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec4 aColor; // Changed to vec4 for RGBA

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec4 vColor; // Pass color to fragment shader

        void main() {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

    std::string fragmentSource = R"(
        #version 330 core
        in vec4 vColor; // Receive color from vertex shader
        out vec4 FragColor;

        void main() {
            FragColor = vColor;
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
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // Changed to 4 floats
    glEnableVertexAttribArray(1);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace WorldGen