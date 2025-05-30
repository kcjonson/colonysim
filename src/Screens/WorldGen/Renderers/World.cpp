#include "World.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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
    : world(nullptr)
    , vao(0)
    , vbo(0)
    , ebo(0)
    , dataGenerated(false)
    , visualizationMode(WorldGen::VisualizationMode::Terrain)
{
}

World::~World()
{
    // Clean up OpenGL resources if necessary
    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
    }
    
    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
    }
    
    if (ebo != 0) {
        glDeleteBuffers(1, &ebo);
    }
    
    // Shader object will clean itself up in its destructor
}

void World::SetWorld(const Generators::World* world)
{
    this->world = world;
    dataGenerated = false; // Need to regenerate rendering data
}

void World::SetVisualizationMode(WorldGen::VisualizationMode mode)
{
    visualizationMode = mode;
    // No need to regenerate anything - shader will handle the color change
}

void World::SetPlateData(const std::vector<Generators::Plate>& plates)
{
    plateData = plates;
    plateColors.clear();
    plateColors.reserve(plates.size());
    
    // Pre-calculate colors for each plate with better contrast
    for (size_t i = 0; i < plates.size(); ++i) {
        const auto& plate = plates[i];
        
        // Use plate ID to generate variation
        float variation = (i * 7919) % 100 / 100.0f; // Prime number for good distribution
        
        glm::vec3 color;
        if (plate.size == Generators::PlateSize::Major) {
            // Major plates: darker, more saturated colors
            if (plate.isOceanic) {
                // Dark blue range for major oceanic
                float r = 0.0f + variation * 0.1f;
                float g = 0.1f + variation * 0.1f;
                float b = 0.3f + variation * 0.2f;
                color = glm::vec3(r, g, b);
            } else {
                // Dark brown/green range for major continental
                float r = 0.2f + variation * 0.2f;
                float g = 0.1f + variation * 0.1f;
                float b = 0.0f + variation * 0.1f;
                color = glm::vec3(r, g, b);
            }
        } else {
            // Minor plates: bright, highly visible colors for contrast
            if (plate.isOceanic) {
                // Bright cyan/turquoise for minor oceanic
                float r = 0.3f + variation * 0.3f;
                float g = 0.6f + variation * 0.3f;
                float b = 0.8f + variation * 0.2f;
                color = glm::vec3(r, g, b);
            } else {
                // Bright orange/yellow for minor continental
                float r = 0.8f + variation * 0.2f;
                float g = 0.5f + variation * 0.3f;
                float b = 0.1f + variation * 0.2f;
                color = glm::vec3(r, g, b);
            }
        }
        plateColors.push_back(color);
        
        // Debug output disabled to reduce console spam
    }
    
    // No need to regenerate - colors are handled by shader
}

// SetRenderMode function removed as we now use a fixed rendering mode

void World::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!world) { return; }

    // Generate rendering data if needed
    if (!dataGenerated) {
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

    // Render plate arrows if in plate visualization mode
    if (visualizationMode == WorldGen::VisualizationMode::TectonicPlates && !plateData.empty()) {
        RenderPlateArrows(viewMatrix, projectionMatrix);
    }

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
    if (!world) return;
    
    const auto& tiles = world->GetTiles();
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
    if (!world) {
        return;
    }

    // Debug logging removed
    
    // Validate the tile geometry to catch any issues early
    ValidateTileGeometry();

    // Clear previous data
    vertexData.clear();
    indices.clear();
    tileFanInfo.clear();
    
    // Get the tiles
    const auto& tiles = world->GetTiles();
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
        unsigned int indexOffset = static_cast<unsigned int>(indices.size());
        // No color calculation needed - shader will handle it
        
        // Get the center position
        glm::vec3 centerPos = glm::normalize(tile.GetCenter());
        const float expansionFactor = 1.001f;
        centerPos *= expansionFactor;
          // Store vertex data
        // First the center vertex
        unsigned int centerVertexIndex = vertexOffset;
          vertexData.push_back(centerPos.x);
        vertexData.push_back(centerPos.y);
        vertexData.push_back(centerPos.z);
        vertexData.push_back(centerPos.x); 
        vertexData.push_back(centerPos.y);
        vertexData.push_back(centerPos.z);
        // Instead of color, store tile data as floats
        vertexData.push_back(static_cast<float>(tile.GetTerrainType())); // Terrain type
        vertexData.push_back(static_cast<float>(tile.GetPlateId()));     // Plate ID
        vertexData.push_back(tile.GetElevation());                       // Elevation
        
        vertexOffset++;
        
        // Store indices for the triangle fan
        // First is the center vertex
        indices.push_back(centerVertexIndex);
        
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
            vertexData.push_back(vertexPos.x);
            vertexData.push_back(vertexPos.y);
            vertexData.push_back(vertexPos.z);
            vertexData.push_back(vertexPos.x); // Normal = position for a sphere
            vertexData.push_back(vertexPos.y);
            vertexData.push_back(vertexPos.z);
            // Instead of color, store tile data as floats
            vertexData.push_back(static_cast<float>(tile.GetTerrainType())); // Terrain type
            vertexData.push_back(static_cast<float>(tile.GetPlateId()));     // Plate ID
            vertexData.push_back(tile.GetElevation());                       // Elevation
            
            // Add this vertex to the indices
            indices.push_back(vertexOffset);
            vertexOffset++;
        }
        
        // Close the loop by adding the first perimeter vertex again
        indices.push_back(firstPerimeterIndex);
        
        // Calculate index count (center + perimeter + repeated first vertex)
        unsigned int indexCount = static_cast<unsigned int>(tileVertices.size() + 2);
        
        // Store the tile information for rendering
        TileFanInfo tileInfo;
        tileInfo.startIndex = indexOffset;   // Offset in the indices array
        tileInfo.vertexCount = vertexCount;  // How many vertices in this tile
        tileInfo.indexCount = indexCount;    // How many indices for the fan
          tileFanInfo.push_back(tileInfo);
          // Debug logging removed
    }
    
    // Summary logging removed
    
    // Set up OpenGL objects
    // Create and bind VAO
    if (vao == 0) {
        glGenVertexArrays(1, &vao);
    }
    glBindVertexArray(vao);
    
    // Create and bind VBO
    if (vbo == 0) {
        glGenBuffers(1, &vbo);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);
    
    // Create and bind EBO
    if (ebo == 0) {
        glGenBuffers(1, &ebo);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
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
    if (shader.getProgram() == 0) {
        if (!shader.loadFromFile("Planet/PlanetVertex.glsl", "Planet/PlanetFragment.glsl")) {
            std::cerr << "Failed to load planet shaders" << std::endl;
        }
    }
    
    dataGenerated = true;
}



void World::RenderTiles(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    if (!dataGenerated || shader.getProgram() == 0) {
        return;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // For better planet rendering, we will not use backface culling
    glDisable(GL_CULL_FACE); 
    
    // Use our shader
    shader.use();
    
    // Set up model matrix
    float scale = world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
      // Set the shader uniforms
    GLint useColorAttribLoc = glGetUniformLocation(shader.getProgram(), "useColorAttrib");
    GLint planetColorLoc = glGetUniformLocation(shader.getProgram(), "planetColor");
    
    // Set matrix uniforms using the Shader class methods
    shader.setUniform("model", modelMatrix);
    shader.setUniform("view", viewMatrix);
    shader.setUniform("projection", projectionMatrix);
      // Set light parameters
    GLint lightPosLoc = glGetUniformLocation(shader.getProgram(), "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shader.getProgram(), "lightColor");
    GLint viewPosLoc = glGetUniformLocation(shader.getProgram(), "viewPos");
    
    // Use camera position from view matrix
    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    
    // Position light to better illuminate the sphere
    glm::vec3 lightPos = cameraPos + glm::vec3(5.0f, 5.0f, 5.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    
    // Set visualization mode uniform
    GLint visualizationModeLoc = glGetUniformLocation(shader.getProgram(), "visualizationMode");
    glUniform1i(visualizationModeLoc, static_cast<int>(visualizationMode));
    
    // Set terrain colors uniform array
    GLint terrainColorsLoc = glGetUniformLocation(shader.getProgram(), "terrainColors");
    if (terrainColorsLoc != -1) {
        glm::vec3 terrainColorArray[16];
        for (int i = 0; i < 16; ++i) {
            if (TerrainColors.count(static_cast<TerrainType>(i))) {
                const glm::vec4& color = TerrainColors.at(static_cast<TerrainType>(i));
                terrainColorArray[i] = glm::vec3(color.r, color.g, color.b);
            } else {
                terrainColorArray[i] = glm::vec3(1.0f, 0.0f, 1.0f); // Magenta for missing
            }
        }
        glUniform3fv(terrainColorsLoc, 16, glm::value_ptr(terrainColorArray[0]));
    }
    
    // Set plate colors uniform array
    GLint plateColorsLoc = glGetUniformLocation(shader.getProgram(), "plateColors");
    if (plateColorsLoc != -1 && !plateColors.empty()) {
        glm::vec3 plateColorArray[32];
        for (int i = 0; i < 32; ++i) {
            if (i < plateColors.size()) {
                plateColorArray[i] = plateColors[i];
            } else {
                plateColorArray[i] = glm::vec3(0.5f, 0.5f, 0.5f); // Gray for missing
            }
        }
        glUniform3fv(plateColorsLoc, 32, glm::value_ptr(plateColorArray[0]));
    }
    
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
    
    // Draw each tile as a triangle fan
    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    for (const auto& tileInfo : tileFanInfo) {
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
    //   for (const auto& tileInfo : tileFanInfo) {
    //     RenderTile(tileInfo, isVisible, false);
    // }

      // Reset OpenGL state
    glDepthMask(GL_TRUE);
    glLineWidth(1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    // Unbind shader program
    shader.unbind();
    
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
    unsigned int centerVertexIdx = indices[tileInfo.startIndex];
    glm::vec3 center(vertexData[centerVertexIdx * 9], 
                     vertexData[centerVertexIdx * 9 + 1], 
                     vertexData[centerVertexIdx * 9 + 2]);
                      
    // Check if this tile has at least one visible vertex
    bool anyVertexVisible = isVisible(center);
    if (!anyVertexVisible) {
        // If center isn't visible, check perimeter vertices
        for (unsigned int i = 1; i < tileInfo.indexCount; ++i) {
            unsigned int vertexIdx = indices[tileInfo.startIndex + i];
            glm::vec3 vertex(vertexData[vertexIdx * 9], 
                             vertexData[vertexIdx * 9 + 1], 
                             vertexData[vertexIdx * 9 + 2]);
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

void World::RenderPlateArrows(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (plateData.empty() || !world) return;
    
    // Use the same shader
    shader.use();
    
    // Set up model matrix
    float scale = world->GetRadius();
    glm::mat4 modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
    
    // Set uniforms
    shader.setUniform("model", modelMatrix);
    shader.setUniform("view", viewMatrix);
    shader.setUniform("projection", projectionMatrix);
    
    // Set isArrow flag to true
    GLint isArrowLoc = glGetUniformLocation(shader.getProgram(), "isArrow");
    if (isArrowLoc != -1) {
        glUniform1i(isArrowLoc, 1); // true
    }
    
    // Set up OpenGL state for arrows
    glEnable(GL_DEPTH_TEST);
    glLineWidth(3.0f);
    
    // Calculate camera forward direction for visibility culling
    glm::mat4 cameraMatrix = glm::inverse(viewMatrix);
    glm::vec3 cameraForward = -glm::normalize(glm::vec3(cameraMatrix[2]));
    
    // Generate and render arrows for each plate
    for (const auto& plate : plateData) {
        // Check if plate center is visible (on the front hemisphere)
        glm::vec3 plateCenter = glm::normalize(plate.center);
        if (glm::dot(plateCenter, cameraForward) > 0.1f) {
            continue; // Skip arrows on back side
        }
        
        // Calculate arrow geometry
        glm::vec3 center = plateCenter * 1.05f; // Slightly above surface
        glm::vec3 movement = glm::normalize(plate.movement);
        float arrowLength = 0.15f; // Arrow length relative to planet radius
        
        // Create arrow tip and tail
        glm::vec3 arrowTip = center + movement * arrowLength;
        glm::vec3 arrowTail = center - movement * arrowLength * 0.3f;
        
        // Create arrowhead - two lines forming a "V" shape
        glm::vec3 perpendicular1 = glm::normalize(glm::cross(movement, plateCenter));
        glm::vec3 perpendicular2 = glm::normalize(glm::cross(perpendicular1, movement));
        
        float arrowheadSize = arrowLength * 0.3f;
        glm::vec3 arrowhead1 = arrowTip - movement * arrowheadSize + perpendicular1 * arrowheadSize * 0.5f;
        glm::vec3 arrowhead2 = arrowTip - movement * arrowheadSize + perpendicular2 * arrowheadSize * 0.5f;
        glm::vec3 arrowhead3 = arrowTip - movement * arrowheadSize - perpendicular1 * arrowheadSize * 0.5f;
        glm::vec3 arrowhead4 = arrowTip - movement * arrowheadSize - perpendicular2 * arrowheadSize * 0.5f;
        
        // Create vertex data for the arrow (shaft + arrowhead)
        std::vector<glm::vec3> arrowVertices = {
            // Arrow shaft
            arrowTail, arrowTip,
            // Arrowhead lines
            arrowTip, arrowhead1,
            arrowTip, arrowhead2,
            arrowTip, arrowhead3,
            arrowTip, arrowhead4
        };
        
        // Create dummy normal and tile data (not used for arrows)
        std::vector<glm::vec3> arrowNormals(arrowVertices.size(), glm::vec3(0, 1, 0));
        std::vector<glm::vec3> arrowTileData(arrowVertices.size(), glm::vec3(0, 0, 0));
        
        // Create temporary VBO for arrow rendering to avoid corrupting world geometry
        GLuint tempVBO, tempVAO;
        glGenVertexArrays(1, &tempVAO);
        glGenBuffers(1, &tempVBO);
        
        glBindVertexArray(tempVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tempVBO);
        
        // Calculate total size needed
        size_t totalSize = arrowVertices.size() * 3 * sizeof(float) + 
                          arrowNormals.size() * 3 * sizeof(float) + 
                          arrowTileData.size() * 3 * sizeof(float);
        
        glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_DYNAMIC_DRAW);
        
        // Upload positions
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                       arrowVertices.size() * 3 * sizeof(float), 
                       arrowVertices.data());
        
        // Upload normals  
        glBufferSubData(GL_ARRAY_BUFFER, 
                       arrowVertices.size() * 3 * sizeof(float),
                       arrowNormals.size() * 3 * sizeof(float),
                       arrowNormals.data());
        
        // Upload tile data
        glBufferSubData(GL_ARRAY_BUFFER, 
                       arrowVertices.size() * 3 * sizeof(float) + arrowNormals.size() * 3 * sizeof(float),
                       arrowTileData.size() * 3 * sizeof(float),
                       arrowTileData.data());
        
        // Set up vertex attributes
        size_t stride = 3 * sizeof(float);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, 
                             (void*)(arrowVertices.size() * 3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Tile data attribute
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, 
                             (void*)(arrowVertices.size() * 3 * sizeof(float) + arrowNormals.size() * 3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        
        // Draw the arrow as lines
        glDrawArrays(GL_LINES, 0, arrowVertices.size());
        
        // Clean up temporary buffers
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &tempVAO);
        glDeleteBuffers(1, &tempVBO);
    }
    
    // Reset isArrow flag
    if (isArrowLoc != -1) {
        glUniform1i(isArrowLoc, 0); // false
    }
    
    // Reset line width
    glLineWidth(1.0f);
    
    // Restore original VAO state (don't unbind the main world VAO)
    glBindVertexArray(vao);
    
    // Unbind shader
    shader.unbind();
}


} // namespace Renderers
} // namespace WorldGen