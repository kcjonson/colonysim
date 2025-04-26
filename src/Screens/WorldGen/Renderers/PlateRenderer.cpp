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
    m_thicknessCacheDirty = false;
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[PlateRenderer] Rebuilt thickness line cache: " << numLines << " lines in " << ms << " ms\n";
}

void PlateRenderer::render(
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices, // Added
    const glm::mat4& modelMatrix,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix)
{
    if (planetVertices.empty() || plates.empty()) return;

    // --- Draw faint plate regions (unique color per plate) ---
    std::vector<glm::vec3> regionVertices;
    std::vector<glm::vec4> regionColors;
    for (const auto& plate : plates) {
        glm::vec3 color = PlateIdToColor(plate->GetId(), (int)plates.size());
        glm::vec4 colorWithAlpha = glm::vec4(glm::mix(color, glm::vec3(1.0f), 0.3f), 0.5f); // More visible, alpha=0.5
        for (int idx : plate->GetVertexIndices()) {
            if (idx >= 0 && idx < planetVertices.size()) {
                regionVertices.push_back(planetVertices[idx]);
                regionColors.push_back(colorWithAlpha);
            }
        }
    }
    if (!regionVertices.empty()) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glPointSize(6.0f);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, regionVertices.size() * sizeof(glm::vec3), regionVertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
        glBufferData(GL_ARRAY_BUFFER, regionColors.size() * sizeof(glm::vec4), regionColors.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(regionVertices.size()));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glPointSize(1.0f);
        glUseProgram(0);
        glDisable(GL_BLEND);
    }

    // --- Draw thickness lines (existing code) ---
    if (m_thicknessCacheDirty) {
        updateThicknessLineCache(plates, planetVertices);
    }
    if (!m_thicknessLineVertices.empty()) {
        glUseProgram(m_shaderProgram);
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glLineWidth(2.0f);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_thicknessLineVertices.size() * sizeof(glm::vec3), m_thicknessLineVertices.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
        glBufferData(GL_ARRAY_BUFFER, m_thicknessLineColors.size() * sizeof(glm::vec4), m_thicknessLineColors.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_thicknessLineVertices.size()));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glLineWidth(1.0f);
        glUseProgram(0);
    }

    // --- Draw true boundary lines (at top plate edge) ---
    // Disabled: Do not draw plate boundary lines (pink/magenta)
    // std::vector<glm::vec3> boundaryLineVertices;
    // std::vector<glm::vec4> boundaryLineColors;
    // for (const auto& plate : plates) {
    //     for (const auto& boundary : plate->GetBoundaries()) {
    //         int plate1Id = plate->GetId();
    //         int plate2Id = (boundary.plate1Index == plate1Id) ? boundary.plate2Index : boundary.plate1Index;
    //         if (plate1Id >= plate2Id) continue; // Only draw once per boundary
    //         // Determine which plate is on top at this boundary
    //         const TectonicPlate* topPlate = nullptr;
    //         if (plate->GetType() == PlateType::Continental) {
    //             topPlate = plate.get();
    //         } else {
    //             // If both oceanic, pick younger (lower avg age at boundary)
    //             const TectonicPlate* other = nullptr;
    //             for (const auto& p : plates) if (p->GetId() == plate2Id) other = p.get();
    //             if (other && other->GetType() == PlateType::Continental) {
    //                 topPlate = other;
    //             } else if (other) {
    //                 float age1 = 0, age2 = 0;
    //                 int n1 = 0, n2 = 0;
    //                 for (int idx : boundary.m_sharedVertexIndices) {
    //                     if (plate->GetVertexCrustAge(idx) > 0) { age1 += plate->GetVertexCrustAge(idx); n1++; }
    //                     if (other->GetVertexCrustAge(idx) > 0) { age2 += other->GetVertexCrustAge(idx); n2++; }
    //                 }
    //                 float avg1 = n1 ? age1/n1 : 1e6f;
    //                 float avg2 = n2 ? age2/n2 : 1e6f;
    //                 topPlate = (avg1 < avg2) ? plate.get() : other;
    //             }
    //         }
    //         // Draw the boundary line for all edges in the boundary
    //         for (const auto& edge : boundary.m_sharedEdgeIndices) {
    //             int u = edge.first, v = edge.second;
    //             if (u >= 0 && u < planetVertices.size() && v >= 0 && v < planetVertices.size()) {
    //                 boundaryLineVertices.push_back(planetVertices[u]);
    //                 boundaryLineVertices.push_back(planetVertices[v]);
    //                 boundaryLineColors.push_back(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f)); // Bright magenta, fully opaque
    //                 boundaryLineColors.push_back(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    //             }
    //         }
    //     }
    // }
    // if (!boundaryLineVertices.empty()) {
    //     glEnable(GL_BLEND);
    //     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //     glUseProgram(m_shaderProgram);
    //     glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    //     glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    //     glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    //     glLineWidth(5.0f);
    //     glBindVertexArray(m_vao);
    //     glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    //     glBufferData(GL_ARRAY_BUFFER, boundaryLineVertices.size() * sizeof(glm::vec3), boundaryLineVertices.data(), GL_DYNAMIC_DRAW);
    //     glBindBuffer(GL_ARRAY_BUFFER, m_colorVbo);
    //     glBufferData(GL_ARRAY_BUFFER, boundaryLineColors.size() * sizeof(glm::vec4), boundaryLineColors.data(), GL_DYNAMIC_DRAW);
    //     glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(boundaryLineVertices.size()));
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);
    //     glBindVertexArray(0);
    //     glLineWidth(1.0f);
    //     glUseProgram(0);
    //     glDisable(GL_BLEND);
    // }
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