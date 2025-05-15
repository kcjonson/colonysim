#define GLM_ENABLE_EXPERIMENTAL
#include "World.h"
#include "../Core/WorldGenParameters.h"
#include <glm/gtx/hash.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <cmath>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <array>
#include <unordered_map>
#include <glm/gtc/noise.hpp>

namespace WorldGen {
namespace Generators {

// Helper function to generate a unique key for the edge between two vertices
uint64_t EdgeKey(int v1, int v2) {
    // Ensure v1 <= v2 for consistent keys
    if (v1 > v2) std::swap(v1, v2);
    return (static_cast<uint64_t>(v1) << 32) | static_cast<uint64_t>(v2);
}

World::World()
    : m_radius(1.0f)
    , m_pentagonCount(0)
    , m_seed(12345) // Default seed
{
    CreateIcosahedron();
}

World::World(const PlanetParameters& params)
    : m_radius(params.radius)
    , m_pentagonCount(0)
    , m_seed(params.seed)
{
    CreateIcosahedron();
}

void World::Generate(int subdivisionLevel, float distortionFactor) {
    std::cout << "Generating world with subdivision level: " << subdivisionLevel 
              << ", distortion: " << distortionFactor << std::endl;
              
    // Create the base icosahedron
    CreateIcosahedron();
    
    // Subdivide it the specified number of times
    std::cout << "Subdividing icosahedron..." << std::endl;
    SubdivideIcosahedron(subdivisionLevel, distortionFactor);
    
    // Convert the triangular mesh to a dual polyhedron of pentagons and hexagons
    std::cout << "Converting to tiles..." << std::endl;
    TrianglesToTiles();
    
    // Set up neighborhood relationships between tiles
    std::cout << "Setting up tile neighbors..." << std::endl;
    SetupTileNeighbors();
    
    // Generate terrain data for the tiles
    std::cout << "Generating terrain data..." << std::endl;
    GenerateTerrainData();
    
    // Log completion
    std::cout << "World generation complete. Generated " << m_tiles.size() 
              << " tiles (" << m_pentagonCount << " pentagons)" << std::endl;
}

void World::CreateIcosahedron() {
    // Create the 12 vertices of the icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio
    const float norm = std::sqrt(1.0f + t*t); // Normalization factor
    
    // Clear and reserve space
    m_icosahedronVertices.clear();
    m_icosahedronVertices.reserve(12);
    
    // Add vertices (normalized to unit sphere)
    // The icosahedron has 12 vertices with coordinates based on combinations
    // of (0, ±1, ±φ) and (±1, ±φ, 0) and (±φ, 0, ±1)
    // where φ is the golden ratio
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(-1.0f, t, 0.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(1.0f, t, 0.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(-1.0f, -t, 0.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(1.0f, -t, 0.0f)));
    
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, t)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, t)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, -t)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, -t)));
    
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(t, 0.0f, -1.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(t, 0.0f, 1.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(-t, 0.0f, -1.0f)));
    m_icosahedronVertices.push_back(glm::normalize(glm::vec3(-t, 0.0f, 1.0f)));    // Create the 20 triangular faces
    m_icosahedronFaces.clear();
    m_icosahedronFaces.reserve(20);
    
    // 5 faces around vertex 0
    m_icosahedronFaces.push_back(std::array<int, 3>{0, 11, 5});
    m_icosahedronFaces.push_back(std::array<int, 3>{0, 5, 1});
    m_icosahedronFaces.push_back(std::array<int, 3>{0, 1, 7});
    m_icosahedronFaces.push_back(std::array<int, 3>{0, 7, 10});
    m_icosahedronFaces.push_back(std::array<int, 3>{0, 10, 11});
    
    // 5 faces adjacent to the faces above
    m_icosahedronFaces.push_back(std::array<int, 3>{1, 5, 9});
    m_icosahedronFaces.push_back(std::array<int, 3>{5, 11, 4});
    m_icosahedronFaces.push_back(std::array<int, 3>{11, 10, 2});
    m_icosahedronFaces.push_back(std::array<int, 3>{10, 7, 6});
    m_icosahedronFaces.push_back(std::array<int, 3>{7, 1, 8});
    
    // 5 faces around vertex 3
    m_icosahedronFaces.push_back(std::array<int, 3>{3, 9, 4});
    m_icosahedronFaces.push_back(std::array<int, 3>{3, 4, 2});
    m_icosahedronFaces.push_back(std::array<int, 3>{3, 2, 6});
    m_icosahedronFaces.push_back(std::array<int, 3>{3, 6, 8});
    m_icosahedronFaces.push_back(std::array<int, 3>{3, 8, 9});
    
    // 5 faces adjacent to the faces above
    m_icosahedronFaces.push_back(std::array<int, 3>{4, 9, 5});
    m_icosahedronFaces.push_back(std::array<int, 3>{2, 4, 11});
    m_icosahedronFaces.push_back(std::array<int, 3>{6, 2, 10});
    m_icosahedronFaces.push_back(std::array<int, 3>{8, 6, 7});
    m_icosahedronFaces.push_back(std::array<int, 3>{9, 8, 1});

    // Initialize subdivision data with the icosahedron
    m_subdivisionVertices = m_icosahedronVertices;
    m_subdivisionFaces = m_icosahedronFaces;
}

void World::SubdivideIcosahedron(int level, float distortionFactor) {
    for (int i = 0; i < level; i++) {
        std::vector<std::array<int, 3>> newFaces;
        m_midPointCache.clear();
        
        for (const auto& face : m_subdivisionFaces) {            // Get the three vertices of the face
            int v1 = face[0];
            int v2 = face[1];
            int v3 = face[2];
            
            // Get the midpoints of the three edges
            int a = GetMidPointIndex(v1, v2, distortionFactor);
            int b = GetMidPointIndex(v2, v3, distortionFactor);
            int c = GetMidPointIndex(v3, v1, distortionFactor);
              // Create four new faces (subdividing the original triangle)
            std::array<int, 3> face1 = {v1, a, c};
            std::array<int, 3> face2 = {v2, b, a};
            std::array<int, 3> face3 = {v3, c, b};
            std::array<int, 3> face4 = {a, b, c};
            newFaces.push_back(face1);
            newFaces.push_back(face2);
            newFaces.push_back(face3);
            newFaces.push_back(face4);
        }
        
        // Replace the old faces with the new ones
        m_subdivisionFaces = std::move(newFaces);
    }
}

int World::GetMidPointIndex(int v1, int v2, float distortionFactor) {
    // First check if we've already calculated this midpoint
    uint64_t key = EdgeKey(v1, v2);
    auto it = m_midPointCache.find(key);
    if (it != m_midPointCache.end()) {
        return static_cast<int>(it->second);
    }
    
    // Calculate the midpoint with distortion
    glm::vec3 midPoint = GetMidPoint(
        m_subdivisionVertices[v1], 
        m_subdivisionVertices[v2], 
        distortionFactor
    );
      // Add the new vertex
    int index = static_cast<int>(m_subdivisionVertices.size());
    m_subdivisionVertices.push_back(midPoint);
      // Store in the cache
    m_midPointCache[key] = static_cast<size_t>(index);
    
    return index;
}

glm::vec3 World::GetMidPoint(const glm::vec3& v1, const glm::vec3& v2, float distortionFactor) {
    // Calculate the midpoint
    glm::vec3 midPoint = (v1 + v2) * 0.5f;
    
    // Apply distortion if factor > 0
    if (distortionFactor > 0.0f) {
        midPoint = ApplyDistortion(midPoint, distortionFactor);
    }
    
    // Project back to the unit sphere
    return glm::normalize(midPoint);
}

glm::vec3 World::ApplyDistortion(const glm::vec3& point, float magnitude) {    // Create a random number generator with the seed
    static std::mt19937 rng(static_cast<unsigned int>(m_seed & 0xFFFFFFFF));
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    // Calculate a random offset vector
    glm::vec3 offset(dist(rng), dist(rng), dist(rng));
    
    // Make the offset perpendicular to the point direction
    // This ensures the distortion doesn't change the distance from center too much
    offset = glm::normalize(offset - point * glm::dot(offset, point));
    
    // Scale by magnitude (which is already scaled by distortionFactor)
    offset *= magnitude * 0.05f; // Scale down to reasonable values
    
    // Apply the distortion
    return point + offset;
}

void World::TrianglesToTiles() {
    // Reset tile data
    m_tiles.clear();
    m_pentagonCount = 0;
      // Create a mapping from vertices to their adjacent face centers
    std::unordered_map<size_t, std::vector<int>> vertexFaceCenters;
    std::vector<glm::vec3> faceCenters;
    
    // Calculate face centers for all triangular faces
    for (size_t i = 0; i < m_subdivisionFaces.size(); i++) {
        const auto& face = m_subdivisionFaces[i];
          // Calculate the face center by averaging its vertices
        glm::vec3 center = (m_subdivisionVertices[face.at(0)] + 
                           m_subdivisionVertices[face.at(1)] + 
                           m_subdivisionVertices[face.at(2)]) / 3.0f;
        center = glm::normalize(center); // Project to sphere
        
        // Store the center
        faceCenters.push_back(center);
        
        // Record that this face center is adjacent to all three vertices
        int faceCenterIdx = static_cast<int>(faceCenters.size() - 1);
        vertexFaceCenters[face[0]].push_back(faceCenterIdx);
        vertexFaceCenters[face[1]].push_back(faceCenterIdx);
        vertexFaceCenters[face[2]].push_back(faceCenterIdx);
    }
    
    // For each vertex, create a tile using the face centers around it
    for (const auto& [vertexIndex, adjacentCenters] : vertexFaceCenters) {        // Identify the shape of tile (pentagon or hexagon)
        // The original 12 icosahedron vertices will be pentagons, the rest are hexagons
        bool isPentagon = vertexIndex < 12;
        Tile::TileShape shape = isPentagon ? Tile::TileShape::Pentagon : Tile::TileShape::Hexagon;
          // Create a new tile centered at the vertex
        Tile tile(m_subdivisionVertices[vertexIndex], shape);
        
        // We need to order the face centers to form a proper polygon
        std::vector<glm::vec3> orderedVertices;
        
        // For this simplified implementation, we'll just add all adjacent face centers
        // In a real implementation, these would need to be ordered correctly
        for (int centerIdx : adjacentCenters) {
            orderedVertices.push_back(faceCenters[centerIdx]);
        }
        
        // Set the tile vertices
        tile.SetVertices(orderedVertices);
        
        // Add the tile to our collection
        m_tiles.push_back(std::move(tile));
        
        // Count pentagons
        if (isPentagon) m_pentagonCount++;
    }
    
    // Debug check
    if (m_pentagonCount != 12) {
        std::cerr << "Warning: Expected 12 pentagons but got " << m_pentagonCount << std::endl;
    }
}

void World::SetupTileNeighbors() {    // Build a map of vertices to tiles
    std::unordered_map<size_t, std::vector<int>> vertexToTiles;
    
    // For each tile, register it with each of its vertices
    for (size_t tileIdx = 0; tileIdx < m_tiles.size(); tileIdx++) {
        const auto& tile = m_tiles[tileIdx];
          // For each vertex in the tile
        for (const auto& vertex : tile.GetVertices()) {
            // Hash the vertex to get a unique ID
            // Note: In a production system, you would need a more robust way to identify vertices
            size_t vertexHash = std::hash<glm::vec3>{}(vertex);
            vertexToTiles[vertexHash].push_back(static_cast<int>(tileIdx));
        }
    }
    
    // Now establish neighborhood relationships
    for (size_t tileIdx = 0; tileIdx < m_tiles.size(); tileIdx++) {
        auto& tile = m_tiles[tileIdx];
        std::vector<int> neighbors;
          // For each vertex in the tile
        for (const auto& vertex : tile.GetVertices()) {
            size_t vertexHash = std::hash<glm::vec3>{}(vertex);
            
            // All tiles that share this vertex are potential neighbors
            for (int otherTileIdx : vertexToTiles[vertexHash]) {
                // Don't add ourselves as a neighbor
                if (otherTileIdx != static_cast<int>(tileIdx)) {
                    // Check if the tiles also share another vertex (meaning they're adjacent)
                    const auto& otherTile = m_tiles[otherTileIdx];
                    
                    // Count shared vertices
                    int sharedVertices = 0;
                    for (const auto& tileVertex : tile.GetVertices()) {
                        for (const auto& otherVertex : otherTile.GetVertices()) {
                            if (glm::distance(tileVertex, otherVertex) < 0.0001f) {
                                sharedVertices++;
                                break;
                            }
                        }
                    }
                    
                    // If they share at least two vertices, they're neighbors
                    if (sharedVertices >= 2) {
                        neighbors.push_back(otherTileIdx);
                    }
                }
            }
        }
        
        // Remove duplicates
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        
        // Set the neighbors
        tile.SetNeighbors(neighbors);
    }
}

void World::GenerateTerrainData() {
    // Use a simple noise-based approach for terrain generation
    // In a real implementation, this would be much more sophisticated
    
    // Random number generator with seed
    static std::mt19937 rng(12345);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    // Generate elevation data
    for (size_t i = 0; i < m_tiles.size(); i++) {
        glm::vec3 pos = m_tiles[i].GetCenter();
        
        // Simple elevation based on position
        // This creates a simple pattern of high and low points
        float elevation = 0.5f + 0.2f * glm::simplex(glm::vec2(pos.x * 3.0f, pos.z * 3.0f));
        elevation += 0.1f * glm::simplex(glm::vec2(pos.x * 6.0f, pos.z * 6.0f));
        elevation += 0.05f * glm::simplex(glm::vec2(pos.x * 12.0f, pos.z * 12.0f));
        
        // Clamp to valid range
        elevation = glm::clamp(elevation, 0.0f, 1.0f);
        
        m_tiles[i].SetElevation(elevation);
        
        // Simple moisture based on elevation and position
        // Higher elevations are typically drier
        float moisture = 0.7f - (elevation - 0.5f) * 0.4f;
        moisture += 0.15f * glm::simplex(glm::vec2(pos.z * 4.0f, pos.y * 4.0f));
        moisture = glm::clamp(moisture, 0.0f, 1.0f);
        
        m_tiles[i].SetMoisture(moisture);
        
        // Simple temperature based on latitude (y-coordinate)
        // Higher latitudes (closer to poles) are colder
        float latitude = std::asin(pos.y);  // -π/2 to +π/2
        float normalizedLatitude = latitude / (3.14159f / 2.0f);  // -1 to +1
        float temperature = 0.8f - 0.6f * std::abs(normalizedLatitude);
        
        // Temperature also decreases with elevation
        temperature -= elevation * 0.2f;
        
        // Add some noise for variety
        temperature += 0.05f * glm::simplex(glm::vec2(pos.x * 5.0f, pos.z * 5.0f));
        temperature = glm::clamp(temperature, 0.0f, 1.0f);
        
        m_tiles[i].SetTemperature(temperature);
    }
    
    // Smooth the terrain properties by averaging with neighbors
    SmoothTerrainData();
}

void World::SmoothTerrainData() {
    // Create a copy of the current terrain data
    std::vector<float> oldElevations(m_tiles.size());
    std::vector<float> oldMoistures(m_tiles.size());
    std::vector<float> oldTemperatures(m_tiles.size());
    
    for (size_t i = 0; i < m_tiles.size(); i++) {
        oldElevations[i] = m_tiles[i].GetElevation();
        oldMoistures[i] = m_tiles[i].GetMoisture();
        oldTemperatures[i] = m_tiles[i].GetTemperature();
    }
    
    // Smooth the terrain data by averaging with neighbors
    for (size_t i = 0; i < m_tiles.size(); i++) {
        const auto& neighbors = m_tiles[i].GetNeighbors();
        if (neighbors.empty()) continue;
        
        // Calculate average values considering neighbors
        float sumElevation = oldElevations[i];
        float sumMoisture = oldMoistures[i];
        float sumTemperature = oldTemperatures[i];
        int count = 1;  // start with 1 for the tile itself
        
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(m_tiles.size())) {
                sumElevation += oldElevations[neighborIdx];
                sumMoisture += oldMoistures[neighborIdx];
                sumTemperature += oldTemperatures[neighborIdx];
                count++;
            }
        }
        
        // Apply the smoothed values
        m_tiles[i].SetElevation(sumElevation / count);
        m_tiles[i].SetMoisture(sumMoisture / count);
        m_tiles[i].SetTemperature(sumTemperature / count);
    }
}

} // namespace Generators
} // namespace WorldGen