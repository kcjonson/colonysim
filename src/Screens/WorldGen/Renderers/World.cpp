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
#include "../Core/TerrainTypes.h"

namespace WorldGen {
namespace Renderers {

World::World()
    : m_world(nullptr)
    , m_vao(0)
    , m_vbo(0)
    , m_ebo(0)
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
    
    // Shader object will clean itself up in its destructor
}

void World::SetWorld(const Generators::World* world)
{
    m_world = world;
    m_dataGenerated = false; // Need to regenerate rendering data
}

// SetRenderMode function removed as we now use a fixed rendering mode

void World::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_world) { return; }

    // Generate rendering data if needed
    if (!m_dataGenerated) {
        GenerateRenderingData();
    }
    
    // Save current OpenGL state
    GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    
    // Configure OpenGL state for planet rendering
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    RenderTiles(viewMatrix, projectionMatrix);

    // Restore previous OpenGL state
    if (cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
        
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
        
    if (blendEnabled)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
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
      // Use the terrain colors defined in TerrainTypes.h
    // We'll convert vec4 to vec3 as needed
    
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
          // Get the terrain type of this tile and use its color
        TerrainType terrainType = tile.GetTerrainType();
          // Get color from the TerrainColors map, convert vec4 to vec3 (drop alpha)
        const glm::vec4& colorVec4 = TerrainColors.at(terrainType);
        glm::vec3 tileColor = glm::vec3(colorVec4.r, colorVec4.g, colorVec4.b);
        
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
        m_vertexData.push_back(centerPos.x); 
        m_vertexData.push_back(centerPos.y);
        m_vertexData.push_back(centerPos.z);
        m_vertexData.push_back(tileColor.r); 
        m_vertexData.push_back(tileColor.g);
        m_vertexData.push_back(tileColor.b);
        
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
          // Next, add all sorted perimeter vertices with same color as the tile
        unsigned int firstPerimeterIndex = vertexOffset;
        
        for (size_t i = 0; i < sortedVertices.size(); ++i) {
            // Normalize and expand the perimeter vertex slightly
            glm::vec3 vertexPos = glm::normalize(sortedVertices[i].second) * expansionFactor;
            
            // Add vertex
            m_vertexData.push_back(vertexPos.x);
            m_vertexData.push_back(vertexPos.y);
            m_vertexData.push_back(vertexPos.z);
            m_vertexData.push_back(vertexPos.x); // Normal = position for a sphere
            m_vertexData.push_back(vertexPos.y);
            m_vertexData.push_back(vertexPos.z);
            m_vertexData.push_back(tileColor.r);
            m_vertexData.push_back(tileColor.g);
            m_vertexData.push_back(tileColor.b);
            
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
    if (m_shader.getProgram() == 0) {
        if (!m_shader.loadFromFile("Planet/PlanetVertex.glsl", "Planet/PlanetFragment.glsl")) {
            std::cerr << "Failed to load planet shaders" << std::endl;
        }
    }
    
    m_dataGenerated = true;
}



void World::RenderTiles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!m_dataGenerated || m_shader.getProgram() == 0) {
        return;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // For better planet rendering, we will not use backface culling
    glDisable(GL_CULL_FACE); 
    
    // Use our shader
    m_shader.use();
    
    // Set up model matrix
    float scale = m_world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
      // Set the shader uniforms
    GLint useColorAttribLoc = glGetUniformLocation(m_shader.getProgram(), "useColorAttrib");
    GLint planetColorLoc = glGetUniformLocation(m_shader.getProgram(), "planetColor");
    
    // Set matrix uniforms using the Shader class methods
    m_shader.setUniform("model", modelMatrix);
    m_shader.setUniform("view", viewMatrix);
    m_shader.setUniform("projection", projectionMatrix);
      // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(m_shader.getProgram(), "lightPos");
    GLint lightColorLoc = glGetUniformLocation(m_shader.getProgram(), "lightColor");
    GLint viewPosLoc = glGetUniformLocation(m_shader.getProgram(), "viewPos");
    
    // Use camera position from view matrix
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light to better illuminate the sphere
    glm::vec3 lightPos = cameraPos + glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    
    // Calculate camera forward direction from view matrix
    glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
    glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraMatrix[2]));
    
    // Helper lambda: returns true if a point is on the visible hemisphere
    auto isVisible = [&](const glm::vec3& pos) {
        glm::vec3 normalizedPos = glm::normalize(pos);
        // Approximation: consider positions just barely beyond the hemisphere (with a small margin)
        return glm::dot(normalizedPos, cameraForward) < 0.05f;
    };

    // PASS 1: Draw the solid colored tiles
    // Tell shader to use vertex colors (1 for true)
    if (useColorAttribLoc != -1) {
        glUniform1i(useColorAttribLoc, 1);
    }
    
    // Draw each tile as a triangle fan
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (const auto& tileInfo : m_tileFanInfo) {
        RenderTile(tileInfo, isVisible, true);
    }

    
    // PASS 3: Draw tile edges
    // Draw wireframe edges for better tile visibility
    // if (useColorAttribLoc != -1) {
    //     glUniform1i(useColorAttribLoc, 0); // Use uniform color for edges
    // }
    // if (planetColorLoc != -1) {
    //     glUniform3f(planetColorLoc, 0.0f, 0.0f, 0.0f); // Black edges
    // }
    
    // glLineWidth(1.5f);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    // glDepthMask(GL_FALSE); // Disable depth writing for wireframe to ensure all lines are visible
    //   for (const auto& tileInfo : m_tileFanInfo) {
    //     RenderTile(tileInfo, isVisible, false);
    // }

      // Reset OpenGL state
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Unbind shader program
    m_shader.unbind();
    
    // Unbind VAO and current texture
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
      
    // Reset blend state to normal alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
      
    // Re-enable face culling for other objects
    glEnable(GL_CULL_FACE);
}

void World::RenderTile(const TileFanInfo& tileInfo, 
                      const std::function<bool(const glm::vec3&)>& isVisible,
                      bool fillMode)
{
    // Get the center vertex for visibility check
    unsigned int centerVertexIdx = m_indices[tileInfo.startIndex];
    glm::vec3 center(m_vertexData[centerVertexIdx * 9], 
                     m_vertexData[centerVertexIdx * 9 + 1], 
                     m_vertexData[centerVertexIdx * 9 + 2]);
                      
    // Check if this tile has at least one visible vertex
    bool anyVertexVisible = isVisible(center);
    if (!anyVertexVisible) {
        // If center isn't visible, check perimeter vertices
        for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
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
        glDrawElements(
            GL_TRIANGLE_FAN,
            tileInfo.indexCount, 
            GL_UNSIGNED_INT, 
            (void*)(tileInfo.startIndex * sizeof(unsigned int))
        );
    }
}

} // namespace Renderers
} // namespace WorldGen