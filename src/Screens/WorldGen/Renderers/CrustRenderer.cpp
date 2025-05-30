#include "CrustRenderer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <functional> // for std::hash
#include <algorithm> // for std::sort
#include <array> // for std::array
#include <set> // for std::set
#include <tuple> // for std::tuple

namespace WorldGen {

CrustRenderer::CrustRenderer()
    : m_vao(0)
    , m_vbo(0)
    , m_ibo(0)
    , m_shaderProgram(0)
    , m_viewportWidth(800)
    , m_viewportHeight(600)
    , m_enabled(true)  // Enable crust rendering by default
    , m_geometryCacheDirty(true) {  // Mark cache dirty initially
}

CrustRenderer::~CrustRenderer() {
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_ibo) glDeleteBuffers(1, &m_ibo);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
}

bool CrustRenderer::initialize() {
    if (!compileShaders()) {
        return false;
    }
    
    setupBuffers();
    
    // Get uniform locations right after compilation
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    m_lightDirLoc = glGetUniformLocation(m_shaderProgram, "lightDir");
    m_lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    
    // Debug info about uniform locations
    std::cout << "Crust shader uniform locations:" << std::endl;
    std::cout << "  - model: " << m_modelLoc << std::endl;
    std::cout << "  - view: " << m_viewLoc << std::endl;
    std::cout << "  - projection: " << m_projectionLoc << std::endl;
    std::cout << "  - lightDir: " << m_lightDirLoc << std::endl;
    std::cout << "  - lightColor: " << m_lightColorLoc << std::endl;
    
    // Force shader recompilation if uniforms are invalid
    if (m_modelLoc == -1 || m_viewLoc == -1 || m_projectionLoc == -1 ||
        m_lightDirLoc == -1 || m_lightColorLoc == -1) {
        
        std::cerr << "WARNING: Some CrustRenderer uniform locations are invalid!" << std::endl;
        
        // Try recompiling with default shaders
        if (m_shaderProgram) {
            glDeleteProgram(m_shaderProgram);
            m_shaderProgram = 0;
            
            if (!compileShaders()) {
                return false;
            }
            
            // Try getting uniforms again
            m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
            m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
            m_projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
            m_lightDirLoc = glGetUniformLocation(m_shaderProgram, "lightDir");
            m_lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
            
            std::cout << "After recompilation, uniform locations:" << std::endl;
            std::cout << "  - model: " << m_modelLoc << std::endl;
            std::cout << "  - view: " << m_viewLoc << std::endl;
            std::cout << "  - projection: " << m_projectionLoc << std::endl;
            std::cout << "  - lightDir: " << m_lightDirLoc << std::endl;
            std::cout << "  - lightColor: " << m_lightColorLoc << std::endl;
        }
    }
    
    return true;
}

void CrustRenderer::render(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                           const std::vector<glm::vec3>& planetVertices,
                           const glm::mat4& modelMatrix,
                           const glm::mat4& viewMatrix,
                           const glm::mat4& projectionMatrix) {
    if (!m_enabled || plates.empty()) {
        std::cout << "CrustRenderer::render - Not rendering because " 
                  << (m_enabled ? "plates are empty" : "renderer is disabled") << std::endl;
        return;
    }
    
    // Only regenerate geometry when explicitly marked as dirty
    if (m_geometryCacheDirty) {
        static bool firstRun = true;
        if (!firstRun) {
            // Only log regeneration after the first time
            std::cout << "Regenerating sphere geometry due to plates being explicitly modified" << std::endl;
        }
        firstRun = false;
        
        updateGeometryCache(plates, planetVertices);
        m_geometryCacheDirty = false;
    }

    // Debug information about available data
    std::cout << "CrustRenderer::render - Rendering with:" << std::endl;
    std::cout << "  - Vertices: " << m_vertexData.size() / 9 << std::endl; 
    std::cout << "  - Indices: " << m_indices.size() << std::endl;
    std::cout << "  - VAO: " << m_vao << ", VBO: " << m_vbo << ", IBO: " << m_ibo << std::endl;
    std::cout << "  - Shader Program: " << m_shaderProgram << std::endl;
    
    // Get the current OpenGL state for debugging
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    std::cout << "  - Viewport: [" << viewport[0] << ", " << viewport[1] << ", " 
              << viewport[2] << ", " << viewport[3] << "]" << std::endl;
              
    GLfloat clearColor[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
    std::cout << "  - Clear Color: [" << clearColor[0] << ", " << clearColor[1] << ", "
              << clearColor[2] << ", " << clearColor[3] << "]" << std::endl;
              
    GLint depthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    std::cout << "  - Depth Func: " << depthFunc << " (7=LESS, 3=LEQUAL)" << std::endl;
    
    GLboolean depthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
    std::cout << "  - Depth Write Enabled: " << (depthMask ? "YES" : "NO") << std::endl;
    
    // Debug matrix values
    std::cout << "  - ModelMatrix[3]: " << modelMatrix[3][0] << ", " << modelMatrix[3][1] 
              << ", " << modelMatrix[3][2] << std::endl;
    std::cout << "  - ViewMatrix[3]: " << viewMatrix[3][0] << ", " << viewMatrix[3][1] 
              << ", " << viewMatrix[3][2] << std::endl;
    
    // Check if MVP would place the vertices in valid NDC range
    glm::mat4 mvp = projectionMatrix * viewMatrix * modelMatrix;
    glm::vec4 testVertex = mvp * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    std::cout << "  - Test vertex in NDC: (" << testVertex.x/testVertex.w << ", " 
              << testVertex.y/testVertex.w << ", " << testVertex.z/testVertex.w 
              << ")" << std::endl;
    
    // Make the planet appear
    // Force the most basic, reliable OpenGL state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE); // Ensure depth buffer writing is enabled
    glDisable(GL_CULL_FACE);
    
    // Clear any existing errors
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error at start: " << err << std::endl;
    }

    // Use shader program
    glUseProgram(m_shaderProgram);
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error after glUseProgram: " << err << std::endl;
    }
    
    // Get uniform locations for this shader program
    // This is important because the compileShaders function might have created a new shader
    // program, invalidating our previous uniform locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    m_lightDirLoc = glGetUniformLocation(m_shaderProgram, "lightDir");
    m_lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    
    // Set uniforms
    if (m_modelLoc != -1) {
        glUniformMatrix4fv(m_modelLoc, 1, GL_FALSE, &modelMatrix[0][0]);
    }
    
    if (m_viewLoc != -1) {
        glUniformMatrix4fv(m_viewLoc, 1, GL_FALSE, &viewMatrix[0][0]);
    }
    
    if (m_projectionLoc != -1) {
        glUniformMatrix4fv(m_projectionLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
    }
    
    // Set lighting uniforms
    if (m_lightDirLoc != -1) {
        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.5f));
        glUniform3fv(m_lightDirLoc, 1, &lightDir[0]);
    }
    
    if (m_lightColorLoc != -1) {
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        glUniform3fv(m_lightColorLoc, 1, &lightColor[0]);
    }
    
    // Check for any uniform setting errors
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error after setting uniforms: " << err << std::endl;
    }
    
    // Bind the vertex array object (VAO)
    glBindVertexArray(m_vao);
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error after binding VAO: " << err << std::endl;
    }
    
    // Re-setup vertex attributes if needed
    // This is crucial - the VAO might not have all attributes enabled
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    
    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // Normal attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    
    // Check for attribute setup errors
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error after setting up vertex attributes: " << err << std::endl;
    }
    
    // Bind the element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    if ((err = glGetError()) != GL_NO_ERROR) {
        std::cout << "  - OpenGL error after binding IBO: " << err << std::endl;
    }
    
    // Debug the vertex attributes after setup
    for (GLuint i = 0; i < 3; i++) {
        GLint enabled;
        glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
        std::cout << "  - Attribute " << i << " after setup: " << (enabled ? "YES" : "NO") << std::endl;
    }
    
    // Check the VAO binding after setup
    GLint currentVAO;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
    std::cout << "  - Current VAO binding after setup: " << currentVAO << std::endl;
    
    // Draw the triangles
    if (!m_indices.empty()) {
        std::cout << "Drawing " << m_indices.size() / 3 << " triangles" << std::endl;
        
        // Draw with filled polygons
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
        
        if ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "  - OpenGL error after drawing: " << err << std::endl;
        }
    } else {
        std::cout << "CrustRenderer::render - No indices available for rendering!" << std::endl;
    }
    
    // Reset state
    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}

void CrustRenderer::resize(int width, int height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
}

bool CrustRenderer::compileShaders() {
    // Create shaders for crust visualization
    std::string vertexShaderPath = "shaders/Planet/CrustVertex.glsl";
    std::string fragmentShaderPath = "shaders/Planet/CrustFragment.glsl";
    
    std::ifstream vertexShaderFile(vertexShaderPath);
    std::ifstream fragmentShaderFile(fragmentShaderPath);
    
    if (!vertexShaderFile.is_open() || !fragmentShaderFile.is_open()) {
        std::cerr << "Failed to open crust shader files. Creating default shaders." << std::endl;
        
        // Define default shaders inline if files are not found
        std::string vertexShaderSource = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aColor;
            layout (location = 2) in vec3 aNormal;
            
            out vec3 Color;
            out vec3 Normal;
            out vec3 FragPos;
            
            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            
            void main() {
                FragPos = vec3(model * vec4(aPos, 1.0));
                Color = aColor;
                Normal = mat3(transpose(inverse(model))) * aNormal;
                gl_Position = projection * view * vec4(FragPos, 1.0);
            }
        )";
        
        std::string fragmentShaderSource = R"(
            #version 330 core
            in vec3 Color;
            in vec3 Normal;
            in vec3 FragPos;
            
            out vec4 FragColor;
            
            uniform vec3 lightDir;
            uniform vec3 lightColor;
            
            void main() {
                // Ambient lighting
                float ambientStrength = 0.2;
                vec3 ambient = ambientStrength * lightColor;
                
                // Diffuse lighting
                vec3 norm = normalize(Normal);
                vec3 lightDirection = normalize(-lightDir);
                float diff = max(dot(norm, lightDirection), 0.0);
                vec3 diffuse = diff * lightColor;
                
                // Combine lighting with vertex color
                vec3 result = (ambient + diffuse) * Color;
                FragColor = vec4(result, 1.0);
            }
        )";
        
        // Compile the vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        const char* vShaderCode = vertexShaderSource.c_str();
        glShaderSource(vertexShader, 1, &vShaderCode, NULL);
        glCompileShader(vertexShader);
        
        // Check for vertex shader compile errors
        GLint success;
        GLchar infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            return false;
        }
        
        // Compile the fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        const char* fShaderCode = fragmentShaderSource.c_str();
        glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
        glCompileShader(fragmentShader);
        
        // Check for fragment shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            return false;
        }
        
        // Create shader program
        m_shaderProgram = glCreateProgram();
        glAttachShader(m_shaderProgram, vertexShader);
        glAttachShader(m_shaderProgram, fragmentShader);
        glLinkProgram(m_shaderProgram);
        
        // Check for linking errors
        glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            return false;
        }
        
        // Delete shaders as they're linked into our program and no longer necessary
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }
    else {
        // Read shader code from files
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
    }
    
    // Get uniform locations
    m_modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    m_viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    m_projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    m_lightDirLoc = glGetUniformLocation(m_shaderProgram, "lightDir");
    m_lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    
    return true;
}

void CrustRenderer::setupBuffers() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ibo);
    
    // We'll fill these during updateGeometryCache
}

void CrustRenderer::updateGeometryCache(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                                        const std::vector<glm::vec3>& planetVertices) {
    m_vertexData.clear();
    m_indices.clear();
    
    if (planetVertices.empty() || plates.empty()) {
        return;
    }
    
    // For each vertex, we store:
    // - position (x,y,z)
    // - color (r,g,b)
    // - normal (nx,ny,nz)
    
    // First, calculate adjusted vertices with elevation
    std::vector<glm::vec3> adjustedVertices;
    std::vector<glm::vec3> vertexColors;
    
    for (size_t i = 0; i < planetVertices.size(); i++) {
        // Find which plate this vertex belongs to and calculate elevation
        float elevation = 0.0f;
        TectonicPlate* plate = nullptr;
        
        for (const auto& p : plates) {
            const auto& vertexIndices = p->GetVertexIndices();
            if (std::find(vertexIndices.begin(), vertexIndices.end(), static_cast<int>(i)) != vertexIndices.end()) {
                plate = p.get();
                elevation = calculateElevationAtVertex(plates, static_cast<int>(i), planetVertices[i]);
                break;
            }
        }
        
        // Adjust position based on elevation (scale outward from center)
        // Increase the elevation factor to make terrain features more pronounced
        glm::vec3 adjustedPos = planetVertices[i] * (1.0f + elevation * 0.15f);
        adjustedVertices.push_back(adjustedPos);
        
        // Calculate color based on elevation and plate type
        glm::vec3 color = calculateVertexColor(elevation, plate);
        vertexColors.push_back(color);
    }
    
    // Calculate normals - use position as normal (pointing outward from center)
    std::vector<glm::vec3> normals;
    for (const auto& vertex : adjustedVertices) {
        normals.push_back(glm::normalize(vertex));
    }
    
    // Build the vertex data array
    for (size_t i = 0; i < adjustedVertices.size(); i++) {
        // Position
        m_vertexData.push_back(adjustedVertices[i].x);
        m_vertexData.push_back(adjustedVertices[i].y);
        m_vertexData.push_back(adjustedVertices[i].z);
        
        // Color
        m_vertexData.push_back(vertexColors[i].r);
        m_vertexData.push_back(vertexColors[i].g);
        m_vertexData.push_back(vertexColors[i].b);
        
        // Normal
        m_vertexData.push_back(normals[i].x);
        m_vertexData.push_back(normals[i].y);
        m_vertexData.push_back(normals[i].z);
    }

    // Determine if the vertices are organized in a grid pattern (common for UV-sphere)
    int resolution = 0;
    // Try to detect the resolution by finding the pattern in the vertex positions
    for (int i = 1; i < static_cast<int>(planetVertices.size()); i++) {
        if (glm::length(planetVertices[0] - planetVertices[i]) < 0.01f) {
            resolution = i;
            break;
        }
    }

    // If we couldn't detect resolution, try to estimate it
    if (resolution == 0) {
        resolution = static_cast<int>(sqrt(adjustedVertices.size() / 2));
        std::cout << "Estimated resolution: " << resolution << std::endl;
    } else {
        std::cout << "Detected resolution: " << resolution << std::endl;
    }

    // Create triangles for a solid sphere surface
    if (resolution > 0) {
        // Calculate rows and columns for UV-sphere grid
        int rows = planetVertices.size() / resolution;
        if (rows == 0) rows = resolution; // Fallback

        for (int row = 0; row < rows - 1; row++) {
            for (int col = 0; col < resolution - 1; col++) {
                // Calculate the four corners of a grid cell
                int idx0 = row * resolution + col;
                int idx1 = row * resolution + col + 1;
                int idx2 = (row + 1) * resolution + col;
                int idx3 = (row + 1) * resolution + col + 1;
                
                // Ensure indices are valid
                if (idx0 < adjustedVertices.size() && 
                    idx1 < adjustedVertices.size() && 
                    idx2 < adjustedVertices.size() && 
                    idx3 < adjustedVertices.size()) {
                    
                    // Create two triangles for this quad
                    // Triangle 1: Bottom-left, top-left, bottom-right
                    m_indices.push_back(idx0);
                    m_indices.push_back(idx2);
                    m_indices.push_back(idx1);
                    
                    // Triangle 2: Bottom-right, top-left, top-right
                    m_indices.push_back(idx1);
                    m_indices.push_back(idx2);
                    m_indices.push_back(idx3);
                }
            }
            
            // Connect the last column with the first column (wrap around)
            int idx0 = row * resolution + (resolution - 1);
            int idx1 = row * resolution;
            int idx2 = (row + 1) * resolution + (resolution - 1);
            int idx3 = (row + 1) * resolution;
            
            // Ensure indices are valid
            if (idx0 < adjustedVertices.size() && 
                idx1 < adjustedVertices.size() && 
                idx2 < adjustedVertices.size() && 
                idx3 < adjustedVertices.size()) {
                
                // Create two triangles for this quad
                m_indices.push_back(idx0);
                m_indices.push_back(idx2);
                m_indices.push_back(idx1);
                
                m_indices.push_back(idx1);
                m_indices.push_back(idx2);
                m_indices.push_back(idx3);
            }
        }
    }
    
    // If the above approach didn't generate any triangles, fall back to a simpler method
    if (m_indices.empty()) {
        std::cout << "Warning: Primary triangulation failed, using fallback method" << std::endl;
        // Fallback to icosphere-like triangulation
        for (size_t i = 0; i < adjustedVertices.size(); i++) {
            // Find the nearest vertices to form triangles
            std::vector<std::pair<size_t, float>> distances;
            for (size_t j = 0; j < adjustedVertices.size(); j++) {
                if (j != i) {
                    float dist = glm::distance(adjustedVertices[i], adjustedVertices[j]);
                    distances.push_back({j, dist});
                }
            }
            
            // Sort by distance and use closest vertices to form triangles
            if (distances.size() >= 5) {
                std::sort(distances.begin(), distances.end(), 
                         [](const auto& a, const auto& b) { return a.second < b.second; });
                
                for (size_t j = 0; j < 4 && j + 1 < distances.size(); j++) {
                    unsigned int idx1 = static_cast<unsigned int>(distances[j].first);
                    unsigned int idx2 = static_cast<unsigned int>(distances[j + 1].first);
                    
                    // Only create triangles where all edges are relatively short
                    if (glm::distance(adjustedVertices[idx1], adjustedVertices[idx2]) < 
                        glm::distance(adjustedVertices[i], adjustedVertices[0]) * 2.0f) {
                        m_indices.push_back(static_cast<unsigned int>(i));
                        m_indices.push_back(idx1);
                        m_indices.push_back(idx2);
                    }
                }
            }
        }
    }
    
    std::cout << "Generated " << m_indices.size() / 3 << " triangles for sphere rendering" << std::endl;
    
    // Update the GPU buffers
    glBindVertexArray(m_vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexData.size() * sizeof(float), m_vertexData.data(), GL_DYNAMIC_DRAW);
    
    // Set up vertex attributes
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    if (!m_indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_DYNAMIC_DRAW);
    }
    
    glBindVertexArray(0);
}

float CrustRenderer::calculateElevationAtVertex(const std::vector<std::shared_ptr<TectonicPlate>>& plates,
                                              int vertexIndex, const glm::vec3& vertexPos) {
    // Find which plate this vertex belongs to
    for (const auto& plate : plates) {
        const auto& vertexIndices = plate->GetVertexIndices();
        if (std::find(vertexIndices.begin(), vertexIndices.end(), vertexIndex) != vertexIndices.end()) {
            // Get crust thickness as a base for elevation
            float thickness = plate->GetVertexCrustThickness(vertexIndex);
            
            // Adjust by plate type
            float baseElevation = plate->GetBaseElevation();
            
            // Check if this vertex is near a boundary for mountain formation
            // For boundaries, check if this vertex is in any boundaries of this plate
            bool nearBoundary = false;
            float boundaryEffect = 0.0f;
            
            for (const auto& boundary : plate->GetBoundaries()) {
                for (int sharedVertex : boundary.m_sharedVertexIndices) {
                    if (sharedVertex == vertexIndex) {
                        nearBoundary = true;
                        
                        // Adjust elevation based on boundary type
                        if (boundary.type == BoundaryType::Convergent) {
                            // Mountains at convergent boundaries
                            boundaryEffect = 0.5f * boundary.stress;
                        } 
                        else if (boundary.type == BoundaryType::Divergent) {
                            // Rifts at divergent boundaries
                            boundaryEffect = -0.3f * boundary.stress;
                        }
                        
                        break;
                    }
                }
                if (nearBoundary) break;
            }
            
            // Combine all factors for final elevation
            return baseElevation + thickness * 0.5f + boundaryEffect;
        }
    }
    
    // Default elevation if vertex is not found in any plate
    return 0.0f;
}

glm::vec3 CrustRenderer::calculateVertexColor(float elevation, const TectonicPlate* plate) {
    // Default color if no plate information (ocean blue)
    if (!plate) {
        return glm::vec3(0.0f, 0.3f, 0.8f);
    }
    
    // Base colors with increased saturation and contrast
    const glm::vec3 DEEP_OCEAN = glm::vec3(0.0f, 0.05f, 0.6f);  // Darker, more saturated blue
    const glm::vec3 OCEAN = glm::vec3(0.0f, 0.4f, 0.9f);       // Brighter blue
    const glm::vec3 SHALLOW = glm::vec3(0.1f, 0.6f, 1.0f);      // More vibrant light blue
    const glm::vec3 BEACH = glm::vec3(1.0f, 0.9f, 0.6f);        // Brighter sand color
    const glm::vec3 LOWLAND = glm::vec3(0.2f, 0.7f, 0.2f);      // More vibrant green
    const glm::vec3 HIGHLAND = glm::vec3(0.5f, 0.6f, 0.2f);     // Brighter green-brown
    const glm::vec3 MOUNTAIN = glm::vec3(0.7f, 0.7f, 0.7f);     // Lighter gray
    const glm::vec3 PEAK = glm::vec3(1.0f, 1.0f, 1.0f);         // Pure white (snow)
    
    // Determine color based on elevation
    if (elevation < -0.5f) {
        return DEEP_OCEAN;
    }
    else if (elevation < -0.2f) {
        float t = (elevation + 0.5f) / 0.3f;  // Normalize to 0-1
        return glm::mix(DEEP_OCEAN, OCEAN, t);
    }
    else if (elevation < 0.0f) {
        float t = (elevation + 0.2f) / 0.2f;  // Normalize to 0-1
        return glm::mix(OCEAN, SHALLOW, t);
    }
    else if (elevation < 0.05f) {
        float t = elevation / 0.05f;  // Normalize to 0-1
        return glm::mix(SHALLOW, BEACH, t);
    }
    else if (elevation < 0.2f) {
        float t = (elevation - 0.05f) / 0.15f;  // Normalize to 0-1
        return glm::mix(BEACH, LOWLAND, t);
    }
    else if (elevation < 0.5f) {
        float t = (elevation - 0.2f) / 0.3f;  // Normalize to 0-1
        return glm::mix(LOWLAND, HIGHLAND, t);
    }
    else if (elevation < 0.8f) {
        float t = (elevation - 0.5f) / 0.3f;  // Normalize to 0-1
        return glm::mix(HIGHLAND, MOUNTAIN, t);
    }
    else {
        float t = (elevation - 0.8f) / 0.2f;  // Normalize to 0-1
        return glm::mix(MOUNTAIN, PEAK, std::min(1.0f, t));
    }
}

} // namespace WorldGen