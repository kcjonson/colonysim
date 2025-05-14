#include "World.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <algorithm>   // For std::sort
#include <cmath>       // For std::atan2

namespace WorldGen {
namespace Renderers {

World::World()
    : m_world(nullptr)
    , m_renderMode(RenderMode::TileType)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
    , m_shaderProgram(0)
    , m_dataGenerated(false)
{
}

World::~World()
{
    // Clean up OpenGL resources if necessary
    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
    }
    
    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
    }
    
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
    }
    
    if (m_shaderProgram != 0) {
        glDeleteProgram(m_shaderProgram);
    }
}

void World::SetWorld(const Generators::World* world)
{
    m_world = world;
    m_dataGenerated = false; // Need to regenerate rendering data
}

void World::SetRenderMode(RenderMode mode)
{
    if (m_renderMode != mode) {
        std::cout << "Changing render mode to: ";
        switch (mode) {
            case RenderMode::Wireframe: std::cout << "Wireframe"; break;
            case RenderMode::Solid: std::cout << "Solid"; break;
            case RenderMode::TileType: std::cout << "TileType"; break;
            case RenderMode::Debug: std::cout << "Debug"; break;
        }
        std::cout << std::endl;
        
        m_renderMode = mode;
        
        // Force regeneration of rendering data when switching modes
        // This ensures any mode-specific settings are applied
        m_dataGenerated = false;
    }
}

void World::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_world) { return; }

    // Generate rendering data if needed
    if (!m_dataGenerated) {
        GenerateRenderingData();
    }
    
    // Disable face culling for planet rendering so both sides of triangles are visible
    glDisable(GL_CULL_FACE);
    
    // Enable depth testing to ensure proper rendering of 3D objects
    glEnable(GL_DEPTH_TEST);

    // Render according to the current mode
    switch (m_renderMode) {
        case RenderMode::Wireframe:
            RenderWireframe(viewMatrix, projectionMatrix);
            break;
        case RenderMode::Solid:
            RenderSolid(viewMatrix, projectionMatrix);
            break;
        case RenderMode::TileType:
            RenderByTileType(viewMatrix, projectionMatrix);
            break;
        case RenderMode::Debug:
            RenderDebug(viewMatrix, projectionMatrix);
            break;
    }
    
    // Re-enable face culling for other elements in the scene
    glEnable(GL_CULL_FACE);
}

// Helper function to validate tile geometry before rendering
void World::ValidateTileGeometry()
{
    if (!m_world) return;
    
    const auto& tiles = m_world->GetTiles();
    // Debug logging removed
    
    int invalidTiles = 0;
    int tooFewVertices = 0;
    int nonPlanarTiles = 0;
    int badCenters = 0;
    
    for (size_t i = 0; i < tiles.size(); ++i) {
        const auto& tile = tiles[i];
        const auto& vertices = tile.GetVertices();
        const auto& center = tile.GetCenter();
        
        // Check 1: Ensure tile has at least 3 vertices
        if (vertices.size() < 3) {
            tooFewVertices++;
            continue;
        }
        
        // Check 2: Ensure tile center is normalized
        if (std::abs(glm::length(center) - 1.0f) > 0.001f) {
            badCenters++;
        }
        
        // Calculate average normal for this tile
        glm::vec3 avgNormal(0.0f);
        for (const auto& v : vertices) {
            avgNormal += v; // Add raw positions first
        }
        avgNormal = glm::normalize(avgNormal); // Then normalize the result
        
        // Check 3: Make sure all vertices lie approximately in the same plane
        // For a sphere, they should all be approximately the same distance from center
        bool isPlanar = true;
        float avgDist = 0;
        for (const auto& v : vertices) {
            avgDist += glm::length(v);
        }
        avgDist /= vertices.size();
        
        for (const auto& v : vertices) {
            float dist = glm::length(v);
            if (std::abs(dist - avgDist) > 0.1f) {
                isPlanar = false;
                break;
            }
        }
        
        if (!isPlanar) {
            nonPlanarTiles++;
        }
        
        if (vertices.size() < 3 || !isPlanar || std::abs(glm::length(center) - 1.0f) > 0.001f) {
            invalidTiles++;
        }
    }
      // Debug logging removed
}

void World::GenerateRenderingData()
{
    if (!m_world) {
        return;
    }

    // Debug logging removed
    
    // Validate the tile geometry to catch any issues early
    ValidateTileGeometry();

    // Clear previous data
    m_vertexData.clear();
    m_indices.clear();
    m_tileFanInfo.clear();
    
    // Get the tiles
    const auto& tiles = m_world->GetTiles();
    
    // Define a set of distinct tile colors for better visualization
    std::array<glm::vec3, 12> tileBaseColors = {
        glm::vec3(1.0f, 0.0f, 0.0f),       // Red
        glm::vec3(0.0f, 1.0f, 0.0f),       // Green
        glm::vec3(0.0f, 0.0f, 1.0f),       // Blue
        glm::vec3(1.0f, 1.0f, 0.0f),       // Yellow
        glm::vec3(1.0f, 0.0f, 1.0f),       // Magenta
        glm::vec3(0.0f, 1.0f, 1.0f),       // Cyan
        glm::vec3(0.5f, 0.0f, 0.0f),       // Dark Red
        glm::vec3(0.0f, 0.5f, 0.0f),       // Dark Green
        glm::vec3(0.0f, 0.0f, 0.5f),       // Dark Blue
        glm::vec3(0.5f, 0.5f, 0.0f),       // Dark Yellow
        glm::vec3(0.5f, 0.0f, 0.5f),       // Dark Magenta
        glm::vec3(0.0f, 0.5f, 0.5f)        // Dark Cyan
    };
    
    // NEW APPROACH:
    // 1. Put all vertices for all tiles in a single array
    // 2. For each tile, create indices that point ONLY to its own vertices
    // 3. Assign a unique color to each tile for easier debugging
    
    unsigned int vertexOffset = 0; // Running count of vertices in the buffer
    
    // Process each tile
    for (size_t tileIdx = 0; tileIdx < tiles.size(); ++tileIdx) {
        const auto& tile = tiles[tileIdx];
        const auto& tileVertices = tile.GetVertices();
        
        // Skip invalid tiles
        if (tileVertices.size() < 3) {
            std::cout << "Skipping invalid tile " << tileIdx << " with only " << tileVertices.size() << " vertices" << std::endl;
            continue;
        }
        
        // Number of vertices for this tile (center + perimeter)
        unsigned int vertexCount = static_cast<unsigned int>(tileVertices.size() + 1);
          // Index where this tile's indices start in the indices array
        unsigned int indexOffset = static_cast<unsigned int>(m_indices.size());
        
        // Create a unique color for this tile based on its index
        glm::vec3 tileColor = tileBaseColors[tileIdx % tileBaseColors.size()];
        
        if (tile.GetType() == Generators::Tile::TileType::Pentagon) {
            tileColor = glm::clamp(tileColor * 1.5f, glm::vec3(0.0f), glm::vec3(1.0f));
        }
        
        // Get the center position
        glm::vec3 centerPos = glm::normalize(tile.GetCenter());
        const float expansionFactor = 1.001f;
        centerPos *= expansionFactor;
          // Store vertex data
        // First the center vertex
        unsigned int centerVertexIndex = vertexOffset;
        
        m_vertexData.push_back(centerPos.x);
        m_vertexData.push_back(centerPos.y);
        m_vertexData.push_back(centerPos.z);
        m_vertexData.push_back(centerPos.x); // Normal = position for a sphere
        m_vertexData.push_back(centerPos.y);
        m_vertexData.push_back(centerPos.z);
        m_vertexData.push_back(tileColor.r * 1.2f); // Make center slightly brighter
        m_vertexData.push_back(tileColor.g * 1.2f);
        m_vertexData.push_back(tileColor.b * 1.2f);
        
        vertexOffset++;
        
        // Store indices for the triangle fan
        // First is the center vertex
        m_indices.push_back(centerVertexIndex);
        
        // Sort the perimeter vertices to ensure they form a proper polygon when connected
        std::vector<std::pair<float, glm::vec3>> sortedVertices;
        
        // Define two orthogonal vectors in the tangent plane of the sphere at the center point
        glm::vec3 normal = glm::normalize(centerPos);
        glm::vec3 tangent1 = glm::vec3(1.0f, 0.0f, 0.0f);
        // If the normal is too close to the x-axis, use a different tangent
        if (glm::abs(glm::dot(normal, tangent1)) > 0.9f) {
            tangent1 = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        
        // Ensure tangent1 is perpendicular to normal
        tangent1 = glm::normalize(tangent1 - normal * glm::dot(normal, tangent1));
        // Create second tangent perpendicular to both normal and tangent1
        glm::vec3 tangent2 = glm::normalize(glm::cross(normal, tangent1));
        
        // Calculate angle of each vertex when projected onto the tangent plane
        for (const auto& vertex : tileVertices) {
            glm::vec3 normVertex = glm::normalize(vertex);
            // Project the vertex onto the tangent plane
            glm::vec3 projectedVec = normVertex - normal * glm::dot(normal, normVertex);
            // Calculate the angle in the tangent plane
            float angle = std::atan2(
                glm::dot(projectedVec, tangent2),
                glm::dot(projectedVec, tangent1)
            );
            
            sortedVertices.push_back({angle, vertex});
        }
        
        // Sort by angle to get the vertices in order around the center
        std::sort(sortedVertices.begin(), sortedVertices.end(), 
            [](const std::pair<float, glm::vec3>& a, const std::pair<float, glm::vec3>& b) {
                return a.first < b.first;
            });
        
        // Next, add all sorted perimeter vertices with slightly different brightness
        unsigned int firstPerimeterIndex = vertexOffset;
        
        for (size_t i = 0; i < sortedVertices.size(); ++i) {
            // Normalize and expand the perimeter vertex slightly
            glm::vec3 vertexPos = glm::normalize(sortedVertices[i].second) * expansionFactor;
            
            // Vary brightness to distinguish vertices
            float brightness = 0.8f + (static_cast<float>(i) / sortedVertices.size()) * 0.2f;
            
            // Add vertex
            m_vertexData.push_back(vertexPos.x);
            m_vertexData.push_back(vertexPos.y);
            m_vertexData.push_back(vertexPos.z);
            m_vertexData.push_back(vertexPos.x); // Normal = position for a sphere
            m_vertexData.push_back(vertexPos.y);
            m_vertexData.push_back(vertexPos.z);
            m_vertexData.push_back(tileColor.r * brightness);
            m_vertexData.push_back(tileColor.g * brightness);
            m_vertexData.push_back(tileColor.b * brightness);
            
            // Add this vertex to the indices
            m_indices.push_back(vertexOffset);
            vertexOffset++;
        }
        
        // Close the loop by adding the first perimeter vertex again
        m_indices.push_back(firstPerimeterIndex);
        
        // Calculate index count (center + perimeter + repeated first vertex)
        unsigned int indexCount = static_cast<unsigned int>(tileVertices.size() + 2);
        
        // Store the tile information for rendering
        TileFanInfo tileInfo;
        tileInfo.startIndex = indexOffset;   // Offset in the indices array
        tileInfo.vertexCount = vertexCount;  // How many vertices in this tile
        tileInfo.indexCount = indexCount;    // How many indices for the fan
          m_tileFanInfo.push_back(tileInfo);
          // Debug logging removed
    }
    
    // Summary logging removed
    
    // Set up OpenGL objects
    // Create and bind VAO
    if (m_vao == 0) {
        glGenVertexArrays(1, &m_vao);
    }
    glBindVertexArray(m_vao);
    
    // Create and bind VBO
    if (m_vbo == 0) {
        glGenBuffers(1, &m_vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertexData.size() * sizeof(float), m_vertexData.data(), GL_STATIC_DRAW);
    
    // Create and bind EBO
    if (m_ebo == 0) {
        glGenBuffers(1, &m_ebo);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Color attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    // Load shaders if not already loaded
    if (m_shaderProgram == 0) {
        m_shaderProgram = compileShaders();
    }
    
    m_dataGenerated = true;
}

// Compile and link the shaders
GLuint World::compileShaders() {
    // Define shader paths
    const std::string vertexPath = "shaders/Planet/PlanetVertex.glsl";
    // Use the proper fragment shader
    const std::string fragmentPath = "shaders/Planet/PlanetFragment.glsl";

    // Read the shader source code from files
    std::string vertexCode;
    std::string fragmentCode;

    try {
        // Read the Vertex Shader code
        std::ifstream vShaderFile(vertexPath);
        if (!vShaderFile.is_open()) {
            std::cerr << "ERROR: Could not open vertex shader file: " << vertexPath << std::endl;
            return 0;
        }
        
        std::stringstream vShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        vShaderFile.close();
        vertexCode = vShaderStream.str();

        // Read the Fragment Shader code
        std::ifstream fShaderFile(fragmentPath);
        if (!fShaderFile.is_open()) {
            std::cerr << "ERROR: Could not open fragment shader file: " << fragmentPath << std::endl;
            return 0;
        }
        
        std::stringstream fShaderStream;
        fShaderStream << fShaderFile.rdbuf();
        fShaderFile.close();
        fragmentCode = fShaderStream.str();    }
    catch (std::ifstream::failure&) {
        std::cerr << "ERROR: Could not read shader files" << std::endl;
        return 0;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // Compile shaders
    unsigned int vertex, fragment;
    int success;
    char infoLog[512];

    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);

    // Check for vertex shader compile errors
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        std::cerr << "ERROR: Vertex shader compilation failed\n" << infoLog << std::endl;
        return 0;
    }

    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    // Check for fragment shader compile errors
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        std::cerr << "ERROR: Fragment shader compilation failed\n" << infoLog << std::endl;
        return 0;
    }

    // Link shaders
    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    // Check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR: Shader program linking failed\n" << infoLog << std::endl;
        return 0;
    }

    // Delete the shaders as they're linked into our program and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    std::cout << "Shaders compiled successfully!" << std::endl;
    return program;
}

void World::RenderWireframe(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_dataGenerated || !m_shaderProgram) {
        return;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Use our shader
    glUseProgram(m_shaderProgram);
    
    // Set up model matrix
    float scale = m_world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    
    // Set the shader uniforms
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    GLint useColorAttribLoc = glGetUniformLocation(m_shaderProgram, "useColorAttrib");
    
    // Use camera position from view matrix (inverse of view matrix * origin)
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light near the camera for better wireframe visibility
    glm::vec3 lightPos = cameraPos + glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    
    // Tell shader to use vertex colors (1 for true)
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 1);
    }
    
    // Make lines thicker for better visibility
    glLineWidth(2.0f);
    
    // Draw in wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Disable depth writing for wireframe to ensure all lines are visible
    glDepthMask(GL_FALSE);
    
    // Bind the VAO and draw
    glBindVertexArray(m_vao);
    // Draw points for each vertex (green, larger)
    glPointSize(16.0f);
    GLint useColorAttribLoc2 = glGetUniformLocation(m_shaderProgram, "useColorAttrib");
    GLint planetColorLoc2 = glGetUniformLocation(m_shaderProgram, "planetColor");
    if (useColorAttribLoc2 != -1) {
        glUniform1i(useColorAttribLoc2, 0); // Use uniform color for points
    }
    if (planetColorLoc2 != -1) {
        glUniform3f(planetColorLoc2, 0.0f, 1.0f, 0.0f); // Green
    }    // Calculate camera forward direction and position from view matrix
    glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
    glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraMatrix[2])); // Looking along negative z-axis
    
    // Helper lambda: returns true if a point is on the visible hemisphere
    auto isVisible = [&](const glm::vec3& pos) {
        // For a sphere, we can use a simpler check:
        // If the dot product of (normalized vertex position) and (camera forward) is negative,
        // then the vertex is on the visible hemisphere (facing the camera)
        glm::vec3 normalizedPos = glm::normalize(pos);
        
        // Approximation: consider positions just barely beyond the hemisphere (with a small margin)
        // This prevents visual seams where triangles could be cut off if exactly at the boundary
        return glm::dot(normalizedPos, cameraForward) < 0.05f;
    };
    // Draw only visible points
    std::vector<float> visibleVerts;
    for (size_t i = 0; i < m_vertexData.size(); i += 9) {
        glm::vec3 pos(m_vertexData[i], m_vertexData[i+1], m_vertexData[i+2]);
        if (isVisible(pos)) {
            visibleVerts.push_back(pos.x);
            visibleVerts.push_back(pos.y);
            visibleVerts.push_back(pos.z);
        }
    }
    if (!visibleVerts.empty()) {
        GLuint vVBO = 0, vVAO = 0;
        glGenVertexArrays(1, &vVAO);
        glGenBuffers(1, &vVBO);
        glBindVertexArray(vVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vVBO);
        glBufferData(GL_ARRAY_BUFFER, visibleVerts.size() * sizeof(float), visibleVerts.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(visibleVerts.size() / 3));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDeleteBuffers(1, &vVBO);
        glDeleteVertexArrays(1, &vVAO);
    }
    glPointSize(1.0f);

    // Draw orange center points for pentagon tiles
    glPointSize(20.0f);
    if (planetColorLoc2 != -1) {
        glUniform3f(planetColorLoc2, 1.0f, 0.5f, 0.0f); // Orange
    }
    if (m_world) {
        const auto& tiles = m_world->GetTiles();
        std::vector<glm::vec3> pentCenters;
        std::vector<glm::vec3> hexCenters;
        for (const auto& tile : tiles) {
            if (tile.GetType() == Generators::Tile::TileType::Pentagon) {
                pentCenters.push_back(tile.GetCenter());
            } else if (tile.GetType() == Generators::Tile::TileType::Hexagon) {
                hexCenters.push_back(tile.GetCenter());
            }
        }
        if (!pentCenters.empty()) {
            std::vector<float> centerVerts;
            for (const auto& c : pentCenters) {
                glm::vec3 pos = glm::normalize(c) * 1.001f * m_world->GetRadius();
                if (isVisible(pos)) {
                    centerVerts.push_back(pos.x);
                    centerVerts.push_back(pos.y);
                    centerVerts.push_back(pos.z);
                }
            }
            if (!centerVerts.empty()) {
                GLuint tempVBO = 0, tempVAO = 0;
                glGenVertexArrays(1, &tempVAO);
                glGenBuffers(1, &tempVBO);
                glBindVertexArray(tempVAO);
                glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
                glBufferData(GL_ARRAY_BUFFER, centerVerts.size() * sizeof(float), centerVerts.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(centerVerts.size() / 3));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
                glDeleteBuffers(1, &tempVBO);
                glDeleteVertexArrays(1, &tempVAO);
            }
        }
        // Draw purple center points for hexagon tiles
        if (!hexCenters.empty()) {
            if (planetColorLoc2 != -1) {
                glUniform3f(planetColorLoc2, 0.6f, 0.0f, 1.0f); // Purple
            }
            std::vector<float> centerVerts;
            for (const auto& c : hexCenters) {
                glm::vec3 pos = glm::normalize(c) * 1.001f * m_world->GetRadius();
                if (isVisible(pos)) {
                    centerVerts.push_back(pos.x);
                    centerVerts.push_back(pos.y);
                    centerVerts.push_back(pos.z);
                }
            }
            if (!centerVerts.empty()) {
                GLuint tempVBO = 0, tempVAO = 0;
                glGenVertexArrays(1, &tempVAO);
                glGenBuffers(1, &tempVBO);
                glBindVertexArray(tempVAO);
                glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
                glBufferData(GL_ARRAY_BUFFER, centerVerts.size() * sizeof(float), centerVerts.data(), GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(centerVerts.size() / 3));
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
                glDeleteBuffers(1, &tempVBO);
                glDeleteVertexArrays(1, &tempVAO);
            }
        }
    }
    glPointSize(1.0f);    // Enable face culling so only the front-facing triangles are rendered
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Draw lines (wireframe) but only for visible triangles
    if (useColorAttribLoc2 != -1) {
        glUniform1i(useColorAttribLoc2, 1); // Use vertex color for lines
    }    // Draw visible tiles as triangle fans
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
      // Draw each tile individually using a triangle fan, only rendering visible tiles
    int tilesRendered = 0;
    for (const auto& tileInfo : m_tileFanInfo) {
          // Get the center vertex for this tile to check visibility
        // The vertex index is the value in the m_indices array at the startIndex position
        unsigned int centerVertexIdx = m_indices[tileInfo.startIndex];
        glm::vec3 center(m_vertexData[centerVertexIdx * 9], m_vertexData[centerVertexIdx * 9 + 1], m_vertexData[centerVertexIdx * 9 + 2]);
        
        // Check if this tile has at least one visible vertex
        bool anyVertexVisible = isVisible(center);        if (!anyVertexVisible) {
            // If center isn't visible, check perimeter vertices
            for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
                // Get the actual vertex index from the indices array
                unsigned int vertexIdx = m_indices[tileInfo.startIndex + i];
                glm::vec3 vertex(m_vertexData[vertexIdx * 9], m_vertexData[vertexIdx * 9 + 1], m_vertexData[vertexIdx * 9 + 2]);
                if (isVisible(vertex)) {
                    anyVertexVisible = true;
                    break;
                }
            }
        }
        
        // Only draw tiles with at least one visible vertex
        if (anyVertexVisible) {
            // Draw as a triangle fan with lines
            glLineWidth(2.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDepthMask(GL_FALSE);
              // Debug logging removed
            
            // Draw this tile's triangle fan using glDrawElements
            // Parameters:
            // 1. GL_TRIANGLE_FAN - drawing mode
            // 2. Count of indices to be rendered
            // 3. Type of the values in the indices array
            // 4. Byte offset into the element array buffer (NOT the index value)
            glDrawElements(
                GL_TRIANGLE_FAN,
                tileInfo.indexCount, 
                GL_UNSIGNED_INT, 
                reinterpret_cast<const void*>(tileInfo.startIndex * sizeof(unsigned int))
            );
            
            tilesRendered++;
        }
    }
      // No debug logging for tile count
    
    // Restore polygon mode and line width
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);    // Draw polygon edges for each tile - only for visible edges
    glLineWidth(3.0f);
    if (useColorAttribLoc2 != -1) {
        glUniform1i(useColorAttribLoc2, 1); // Use vertex colors for polygon edges instead of uniform color
    }    // Draw edges using the triangle fan data we already have
    // This simplifies the code and ensures edges match the triangles exactly
    
    // Define a set of distinct colors for triangle identification (same as in GenerateRenderingData)
    std::array<glm::vec3, 6> triangleColors = {
        glm::vec3(1.0f, 0.0f, 0.0f), // Red
        glm::vec3(0.0f, 1.0f, 0.0f), // Green
        glm::vec3(0.0f, 0.0f, 1.0f), // Blue
        glm::vec3(1.0f, 1.0f, 0.0f), // Yellow
        glm::vec3(1.0f, 0.0f, 1.0f), // Magenta
        glm::vec3(0.0f, 1.0f, 1.0f)  // Cyan
    };    // Draw edges for each tile
    int tilesDrawnEdges = 0;
    for (const auto& tileInfo : m_tileFanInfo) {
          // Get the center vertex - use the index value from m_indices
        unsigned int centerVertexIdx = m_indices[tileInfo.startIndex];
        glm::vec3 center(m_vertexData[centerVertexIdx * 9], 
                          m_vertexData[centerVertexIdx * 9 + 1], 
                          m_vertexData[centerVertexIdx * 9 + 2]);
                          
        // Check visibility for this tile
        bool anyVertexVisible = isVisible(center);
        if (!anyVertexVisible) {        // If center isn't visible, check perimeter vertices
            for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
                // Get the actual vertex index from the indices array
                unsigned int vertexIdx = m_indices[tileInfo.startIndex + i];
                glm::vec3 vertex(m_vertexData[vertexIdx * 9], 
                                  m_vertexData[vertexIdx * 9 + 1], 
                                  m_vertexData[vertexIdx * 9 + 2]);
                if (isVisible(vertex)) {
                    anyVertexVisible = true;
                    break;
                }
            }
        }
        
        // Only process visible tiles
        if (anyVertexVisible) {
            // Create lines for this tile
            std::vector<float> tileEdgeVerts;
              // Draw radial edges - from center to each perimeter vertex
            for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
                // Get the actual vertex index from the indices array
                unsigned int vertexIdx = m_indices[tileInfo.startIndex + i];
                
                // Get the perimeter vertex
                glm::vec3 vertex(m_vertexData[vertexIdx * 9], 
                                  m_vertexData[vertexIdx * 9 + 1], 
                                  m_vertexData[vertexIdx * 9 + 2]);
                                  
                // Get the color from the vertex data                  
                glm::vec3 color(m_vertexData[vertexIdx * 9 + 6],
                                 m_vertexData[vertexIdx * 9 + 7],
                                 m_vertexData[vertexIdx * 9 + 8]);
                
                // Add the line from center to this perimeter vertex
                if (isVisible(center) || isVisible(vertex)) {
                    // Center vertex with its color
                    tileEdgeVerts.push_back(center.x); 
                    tileEdgeVerts.push_back(center.y); 
                    tileEdgeVerts.push_back(center.z);
                    tileEdgeVerts.push_back(color.r); 
                    tileEdgeVerts.push_back(color.g); 
                    tileEdgeVerts.push_back(color.b);
                    
                    // Perimeter vertex with its color
                    tileEdgeVerts.push_back(vertex.x); 
                    tileEdgeVerts.push_back(vertex.y); 
                    tileEdgeVerts.push_back(vertex.z);
                    tileEdgeVerts.push_back(color.r); 
                    tileEdgeVerts.push_back(color.g); 
                    tileEdgeVerts.push_back(color.b);
                }
            }            // Draw perimeter edges - connect each perimeter vertex to the next
            for (unsigned int i = 1; i < tileInfo.indexCount - 1; ++i) {
                // Get the actual vertex indices from the indices array
                unsigned int vertexIdx1 = m_indices[tileInfo.startIndex + i];
                
                // Connect to the next vertex in order, or wrap back to the first perimeter vertex
                unsigned int nextIndex = (i == tileInfo.indexCount - 2) ? 1 : i + 1;
                unsigned int vertexIdx2 = m_indices[tileInfo.startIndex + nextIndex];
                
                // Get the vertices
                glm::vec3 v1(m_vertexData[vertexIdx1 * 9], 
                              m_vertexData[vertexIdx1 * 9 + 1], 
                              m_vertexData[vertexIdx1 * 9 + 2]);
                              
                glm::vec3 v2(m_vertexData[vertexIdx2 * 9], 
                              m_vertexData[vertexIdx2 * 9 + 1], 
                              m_vertexData[vertexIdx2 * 9 + 2]);
                
                // Get the color from the vertex data
                glm::vec3 color(m_vertexData[vertexIdx1 * 9 + 6],
                                 m_vertexData[vertexIdx1 * 9 + 7],
                                 m_vertexData[vertexIdx1 * 9 + 8]);
                
                // Add the perimeter edge if at least one endpoint is visible
                if (isVisible(v1) || isVisible(v2)) {
                    // First vertex with its color
                    tileEdgeVerts.push_back(v1.x); tileEdgeVerts.push_back(v1.y); tileEdgeVerts.push_back(v1.z);
                    tileEdgeVerts.push_back(color.r); tileEdgeVerts.push_back(color.g); tileEdgeVerts.push_back(color.b);
                    
                    // Second vertex with its color
                    tileEdgeVerts.push_back(v2.x); tileEdgeVerts.push_back(v2.y); tileEdgeVerts.push_back(v2.z);
                    tileEdgeVerts.push_back(color.r); tileEdgeVerts.push_back(color.g); tileEdgeVerts.push_back(color.b);
                }
            }
            
            // Draw the edges for this tile
            if (!tileEdgeVerts.empty()) {
                GLuint edgeVBO = 0, edgeVAO = 0;
                glGenVertexArrays(1, &edgeVAO);
                glGenBuffers(1, &edgeVBO);
                glBindVertexArray(edgeVAO);
                glBindBuffer(GL_ARRAY_BUFFER, edgeVBO);
                glBufferData(GL_ARRAY_BUFFER, tileEdgeVerts.size() * sizeof(float), tileEdgeVerts.data(), GL_STATIC_DRAW);
                
                // Position attribute (3 floats)
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
                glEnableVertexAttribArray(0);
                
                // Color attribute (3 floats)
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
                glEnableVertexAttribArray(2);
                
                glLineWidth(3.0f); // Thicker lines for better visibility
                glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(tileEdgeVerts.size() / 6));
                
                glBindBuffer(GL_ARRAY_BUFFER, 0);                glBindVertexArray(0);
                glDeleteBuffers(1, &edgeVBO);
                glDeleteVertexArrays(1, &edgeVAO);
            }
        }
        tilesDrawnEdges++;
    }
    glLineWidth(1.0f);
    glBindVertexArray(0);
    
    // Reset OpenGL state
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // At the end, restore culling state if needed
    glDisable(GL_CULL_FACE);
}

void World::RenderSolid(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_dataGenerated || !m_shaderProgram) {
        return;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE); // Disable face culling for consistent rendering
    
    // Use our shader
    glUseProgram(m_shaderProgram);
    
    // Set up model matrix
    float scale = m_world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    
    // Set the shader uniforms
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    GLint useColorAttribLoc = glGetUniformLocation(m_shaderProgram, "useColorAttrib");
    GLint planetColorLoc = glGetUniformLocation(m_shaderProgram, "planetColor");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    
    // Use camera position from view matrix (inverse of view matrix * origin)
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light for optimal viewing
    glm::vec3 lightPos = cameraPos + glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 planetColor = glm::vec3(0.0f, 0.5f, 1.0f); // Ocean blue color
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform3fv(planetColorLoc, 1, glm::value_ptr(planetColor));
    
    // Use shader-defined color instead of vertex color (0 for false)
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 0);
    }
    
    // Draw each tile as a triangle fan
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    
    // Helper lambda: returns true if a point is on the visible hemisphere
    auto isVisible = [&](const glm::vec3& pos) {
        // For a sphere, we can use a simpler check:
        // If the dot product of (normalized vertex position) and (camera forward) is negative,
        // then the vertex is on the visible hemisphere (facing the camera)
        glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
        glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraMatrix[2]));
        glm::vec3 normalizedPos = glm::normalize(pos);
        
        // Approximation: consider positions just barely beyond the hemisphere (with a small margin)
        return glm::dot(normalizedPos, cameraForward) < 0.05f;
    };
      // Draw each tile as a triangle fan
    for (const auto& tileInfo : m_tileFanInfo) {
        // Get the center vertex for visibility check - use indices array for correct lookup
        unsigned int centerVertexIdx = m_indices[tileInfo.startIndex];
        glm::vec3 center(m_vertexData[centerVertexIdx * 9], 
                          m_vertexData[centerVertexIdx * 9 + 1], 
                          m_vertexData[centerVertexIdx * 9 + 2]);
                          
        // Check if this tile has at least one visible vertex
        bool anyVertexVisible = isVisible(center);
        if (!anyVertexVisible) {
            // If center isn't visible, check perimeter vertices
            for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
                // Get the actual vertex index from the indices array
                unsigned int vertexIdx = m_indices[tileInfo.startIndex + i];
                glm::vec3 vertex(m_vertexData[vertexIdx * 9], 
                                  m_vertexData[vertexIdx * 9 + 1], 
                                  m_vertexData[vertexIdx * 9 + 2]);
                if (isVisible(vertex)) {
                    anyVertexVisible = true;
                    break;
                }
            }
        }
        
        // Only draw tiles with at least one visible vertex
        if (anyVertexVisible) {
            // Draw this tile's triangle fan with solid fill
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            
            // Draw this tile's triangle fan
            glDrawElements(
                GL_TRIANGLE_FAN,
                tileInfo.indexCount, 
                GL_UNSIGNED_INT, 
                (void*)(tileInfo.startIndex * sizeof(unsigned int))
            );
        }
    }
    
    glBindVertexArray(0);
    
    // Re-enable face culling if needed for other rendering
    glEnable(GL_CULL_FACE);
}

void World::RenderByTileType(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    // In our new implementation, the tile type coloring is done in the shader
    // using the color attribute set in GenerateRenderingData()
    
    if (!m_dataGenerated || !m_shaderProgram) {
        return;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // For better planet rendering, we will not use backface culling
    glDisable(GL_CULL_FACE); 
    
    // Use our shader
    glUseProgram(m_shaderProgram);
    
    // Set up model matrix
    float scale = m_world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    
    // Set the shader uniforms
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    GLint useColorAttribLoc = glGetUniformLocation(m_shaderProgram, "useColorAttrib");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    
    // Use camera position from view matrix
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light to better illuminate the sphere
    glm::vec3 lightPos = cameraPos + glm::vec3(0.0f, 5.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    
    // Tell shader to use vertex colors (1 for true)
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 1);
    }
    
    // Helper lambda: returns true if a point is on the visible hemisphere
    auto isVisible = [&](const glm::vec3& pos) {
        // For a sphere, we can use a simpler check:
        // If the dot product of (normalized vertex position) and (camera forward) is negative,
        // then the vertex is on the visible hemisphere (facing the camera)
        glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
        glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraMatrix[2]));
        glm::vec3 normalizedPos = glm::normalize(pos);
        
        // Approximation: consider positions just barely beyond the hemisphere (with a small margin)
        return glm::dot(normalizedPos, cameraForward) < 0.05f;
    };
    
    // Draw each tile as a triangle fan
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      // Draw each tile individually using a triangle fan
    for (const auto& tileInfo : m_tileFanInfo) {
        // Get the center vertex for visibility check - use indices array for correct lookup
        unsigned int centerVertexIdx = m_indices[tileInfo.startIndex];
        glm::vec3 center(m_vertexData[centerVertexIdx * 9], 
                          m_vertexData[centerVertexIdx * 9 + 1], 
                          m_vertexData[centerVertexIdx * 9 + 2]);
                          
        // Check if this tile has at least one visible vertex
        bool anyVertexVisible = isVisible(center);
        if (!anyVertexVisible) {
            // If center isn't visible, check perimeter vertices
            for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
                // Get the actual vertex index from the indices array
                unsigned int vertexIdx = m_indices[tileInfo.startIndex + i];
                glm::vec3 vertex(m_vertexData[vertexIdx * 9], 
                                 m_vertexData[vertexIdx * 9 + 1], 
                                 m_vertexData[vertexIdx * 9 + 2]);
                if (isVisible(vertex)) {
                    anyVertexVisible = true;
                    break;
                }
            }
        }
        
        // Only draw tiles with at least one visible vertex
        if (anyVertexVisible) {
            // Draw this tile's triangle fan
            glDrawElements(
                GL_TRIANGLE_FAN,
                tileInfo.indexCount, 
                GL_UNSIGNED_INT, 
                (void*)(tileInfo.startIndex * sizeof(unsigned int))
            );
        }
    }
    
    glBindVertexArray(0);
    
    // Re-enable face culling for other objects if necessary
    glEnable(GL_CULL_FACE);
}

void World::RenderDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_dataGenerated || !m_shaderProgram) {
        return;
    }
    
    // Enable depth testing but disable face culling to see all triangles
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    // Use our shader
    glUseProgram(m_shaderProgram);
    
    // Set up model matrix
    float scale = m_world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    
    // Set the shader uniforms
    GLint modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    GLint viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    GLint useColorAttribLoc = glGetUniformLocation(m_shaderProgram, "useColorAttrib");
    GLint planetColorLoc = glGetUniformLocation(m_shaderProgram, "planetColor");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    
    // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");
    
    // Use camera position from view matrix
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light with camera for consistent illumination
    glm::vec3 lightPos = cameraPos;
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    
    // Our debug rendering uses a multi-pass approach for maximum gap visibility
    
    // PASS 1: Draw a solid black background sphere slightly smaller than our actual sphere
    // This ensures any gaps will appear as black against the colored tiles
    if (planetColorLoc != -1) {
        glUniform3f(planetColorLoc, 0.0f, 0.0f, 0.0f); // Black background
    }
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 0); // Use uniform color
    }
    
    // Make the black sphere slightly smaller (0.995x) to ensure it's behind our tiles
    glm::mat4 backgroundModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale * 0.995f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(backgroundModelMatrix));
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    
    // PASS 2: Draw the actual colored tiles
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    
    // Use vertex colors for the rainbow debug pattern
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 1);
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    
    // PASS 3: Draw wireframe overlay with thicker lines
    if (planetColorLoc != -1) {
        glUniform3f(planetColorLoc, 0.0f, 0.0f, 0.0f); // Black wireframe
    }
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 0); // Use uniform color for wireframe
    }
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(1.5f);
    
    // Enable polygon offset to avoid z-fighting
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    
    // Reset OpenGL state
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glBindVertexArray(0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
}



} // namespace Renderers
} // namespace WorldGen