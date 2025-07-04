// Standard headers
#include <cmath>
#include <random>
#include <unordered_set>
#include <algorithm>
#include <iostream>
#include <array>
#include <unordered_map>

// GLM headers 
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/noise.hpp>

// Project headers
#include "World.h"
#include "Plate.h"
#include "../Core/WorldGenParameters.h"
#include "../Core/Util.h"
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

World::World(const PlanetParameters& params, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker)
    : radius(params.radius)
    , pentagonCount(0)
    , seed(seed)
    , progressTracker(progressTracker)
{
    CreateIcosahedron();
}



void World::Generate(int subdivisionLevel, float distortionFactor, std::shared_ptr<ProgressTracker> progressTracker) {
    // Ensure we have a valid progress tracker
    if (!progressTracker) {
        throw std::invalid_argument("ProgressTracker is required for world generation");
    }
    
    // Store the progress tracker
    this->progressTracker = progressTracker;
    
    // Set up phases if they haven't been configured yet
    if (progressTracker) {
        // Reset the progress tracker
        progressTracker->Reset();
        
        // Add phases with appropriate weights
        progressTracker->AddPhase("Initialization", 0.05f);
        progressTracker->AddPhase("Subdividing", 0.35f);
        progressTracker->AddPhase("Creating Tiles", 0.15f);
        progressTracker->AddPhase("Setting Up Neighbors", 0.15f);
        progressTracker->AddPhase("Generating Terrain", 0.30f);
        
        // Start first phase
        progressTracker->StartPhase("Initialization");
    }
    
    std::cout << "Generating world with subdivision level: " << subdivisionLevel 
              << ", distortion: " << distortionFactor << std::endl;
              
    // Create the base icosahedron
    CreateIcosahedron();
    
    // Report phase completion
    if (progressTracker) {
        progressTracker->CompletePhase();
        progressTracker->StartPhase("Subdividing");
    }
    
    // Subdivide it the specified number of times
    std::cout << "Subdividing icosahedron..." << std::endl;
    SubdivideIcosahedron(subdivisionLevel, distortionFactor);
    
    // Report phase completion
    if (progressTracker) {
        progressTracker->CompletePhase();
        progressTracker->StartPhase("Creating Tiles");
    }
    
    // Convert the triangular mesh to a dual polyhedron of pentagons and hexagons
    std::cout << "Converting to tiles..." << std::endl;
    TrianglesToTiles();
    
    // Report phase completion
    if (progressTracker) {
        progressTracker->CompletePhase();
        progressTracker->StartPhase("Setting Up Neighbors");
    }
    
    // Set up neighborhood relationships between tiles
    std::cout << "Setting up tile neighbors..." << std::endl;
    SetupTileNeighbors();
    
    // Report phase completion
    if (progressTracker) {
        progressTracker->CompletePhase();
        progressTracker->StartPhase("Generating Terrain");
    }
    
    // Skip old terrain generation - plate-based system will handle it
    // Initialize tiles with base values
    InitializeBaseTiles();
    
    // Report phase completion
    if (progressTracker) {
        progressTracker->CompletePhase();
    }
    
    // Log completion
    std::cout << "World generation complete. Generated " << tiles.size() 
              << " tiles (" << pentagonCount << " pentagons)" << std::endl;
}

void World::CreateIcosahedron() {
    // Create the 12 vertices of the icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // Golden ratio
    const float norm = std::sqrt(1.0f + t*t); // Normalization factor
    
    // Clear and reserve space
    icosahedronVertices.clear();
    icosahedronVertices.reserve(12);
    
    // Add vertices (normalized to unit sphere)
    // The icosahedron has 12 vertices with coordinates based on combinations
    // of (0, ±1, ±φ) and (±1, ±φ, 0) and (±φ, 0, ±1)
    // where φ is the golden ratio
    icosahedronVertices.push_back(glm::normalize(glm::vec3(-1.0f, t, 0.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(1.0f, t, 0.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(-1.0f, -t, 0.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(1.0f, -t, 0.0f)));
    
    icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, t)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, t)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, -t)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, -t)));
    
    icosahedronVertices.push_back(glm::normalize(glm::vec3(t, 0.0f, -1.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(t, 0.0f, 1.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(-t, 0.0f, -1.0f)));
    icosahedronVertices.push_back(glm::normalize(glm::vec3(-t, 0.0f, 1.0f)));    // Create the 20 triangular faces
    icosahedronFaces.clear();
    icosahedronFaces.reserve(20);
    
    // 5 faces around vertex 0
    icosahedronFaces.push_back(std::array<int, 3>{0, 11, 5});
    icosahedronFaces.push_back(std::array<int, 3>{0, 5, 1});
    icosahedronFaces.push_back(std::array<int, 3>{0, 1, 7});
    icosahedronFaces.push_back(std::array<int, 3>{0, 7, 10});
    icosahedronFaces.push_back(std::array<int, 3>{0, 10, 11});
    
    // 5 faces adjacent to the faces above
    icosahedronFaces.push_back(std::array<int, 3>{1, 5, 9});
    icosahedronFaces.push_back(std::array<int, 3>{5, 11, 4});
    icosahedronFaces.push_back(std::array<int, 3>{11, 10, 2});
    icosahedronFaces.push_back(std::array<int, 3>{10, 7, 6});
    icosahedronFaces.push_back(std::array<int, 3>{7, 1, 8});
    
    // 5 faces around vertex 3
    icosahedronFaces.push_back(std::array<int, 3>{3, 9, 4});
    icosahedronFaces.push_back(std::array<int, 3>{3, 4, 2});
    icosahedronFaces.push_back(std::array<int, 3>{3, 2, 6});
    icosahedronFaces.push_back(std::array<int, 3>{3, 6, 8});
    icosahedronFaces.push_back(std::array<int, 3>{3, 8, 9});
    
    // 5 faces adjacent to the faces above
    icosahedronFaces.push_back(std::array<int, 3>{4, 9, 5});
    icosahedronFaces.push_back(std::array<int, 3>{2, 4, 11});
    icosahedronFaces.push_back(std::array<int, 3>{6, 2, 10});
    icosahedronFaces.push_back(std::array<int, 3>{8, 6, 7});
    icosahedronFaces.push_back(std::array<int, 3>{9, 8, 1});

    // Initialize subdivision data with the icosahedron
    subdivisionVertices = icosahedronVertices;
    subdivisionFaces = icosahedronFaces;
}

void World::SubdivideIcosahedron(int level, float distortionFactor) {
    for (int i = 0; i < level; i++) {
        // Report subdivision progress if we have a tracker
        if (progressTracker) {
            float iterationProgress = static_cast<float>(i) / level;
            std::string message = "Subdividing icosphere (level " + std::to_string(i+1) + 
                                 " of " + std::to_string(level) + ")";
            progressTracker->UpdateProgress(iterationProgress, message);
        }

        std::vector<std::array<int, 3>> newFaces;
        midPointCache.clear();
        
        // Count for more detailed progress reporting within each level
        size_t faceCount = subdivisionFaces.size();
        size_t facesDone = 0;
        
        for (const auto& face : subdivisionFaces) {
            // Get the three vertices of the face
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
            
            // Report detailed progress for large subdivision levels
            facesDone++;
            if (progressTracker && level > 3 && facesDone % 100 == 0) {
                float subProgress = static_cast<float>(i) / level + 
                                   (static_cast<float>(facesDone) / faceCount) / level;
                std::string detailMsg = "Processing face " + std::to_string(facesDone) + 
                                      " of " + std::to_string(faceCount);
                progressTracker->UpdateProgress(subProgress, detailMsg);
            }
        }
        
        // Replace the old faces with the new ones
        subdivisionFaces = std::move(newFaces);
    }
}

int World::GetMidPointIndex(int v1, int v2, float distortionFactor) {
    // First check if we've already calculated this midpoint
    uint64_t key = EdgeKey(v1, v2);
    auto it = midPointCache.find(key);
    if (it != midPointCache.end()) {
        return static_cast<int>(it->second);
    }
    
    // Calculate the midpoint with distortion
    glm::vec3 midPoint = GetMidPoint(
        subdivisionVertices[v1], 
        subdivisionVertices[v2], 
        distortionFactor
    );
      // Add the new vertex
    int index = static_cast<int>(subdivisionVertices.size());
    subdivisionVertices.push_back(midPoint);
      // Store in the cache
    midPointCache[key] = static_cast<size_t>(index);
    
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

glm::vec3 World::ApplyDistortion(const glm::vec3& point, float magnitude) {
    // Create a random number generator with the seed (non-static so it's re-seeded each time)
    std::mt19937 rng(static_cast<unsigned int>(seed & 0xFFFFFFFF));
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
    tiles.clear();
    pentagonCount = 0;
      // Create a mapping from vertices to their adjacent face centers
    std::unordered_map<size_t, std::vector<int>> vertexFaceCenters;
    std::vector<glm::vec3> faceCenters;
    
    // Calculate face centers for all triangular faces
    size_t totalFaces = subdivisionFaces.size();
    size_t processedFaces = 0;
    
    for (size_t i = 0; i < totalFaces; i++) {
        const auto& face = subdivisionFaces[i];
          // Calculate the face center by averaging its vertices
        glm::vec3 center = (subdivisionVertices[face.at(0)] + 
                           subdivisionVertices[face.at(1)] + 
                           subdivisionVertices[face.at(2)]) / 3.0f;
        center = glm::normalize(center); // Project to sphere
        
        // Store the center
        faceCenters.push_back(center);
        
        // Record that this face center is adjacent to all three vertices
        int faceCenterIdx = static_cast<int>(faceCenters.size() - 1);
        vertexFaceCenters[face[0]].push_back(faceCenterIdx);
        vertexFaceCenters[face[1]].push_back(faceCenterIdx);
        vertexFaceCenters[face[2]].push_back(faceCenterIdx);
        
        // Report progress periodically
        processedFaces++;
        if (progressTracker && processedFaces % 500 == 0) {
            float progress = static_cast<float>(processedFaces) / totalFaces;
            std::string message = "Calculating face centers (" + 
                                 std::to_string(processedFaces) + " of " + 
                                 std::to_string(totalFaces) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // For each vertex, create a tile using the face centers around it
    size_t totalVertices = vertexFaceCenters.size();
    size_t processedVertices = 0;
    
    for (const auto& [vertexIndex, adjacentCenters] : vertexFaceCenters) {        
        // Identify the shape of tile (pentagon or hexagon)
        // The original 12 icosahedron vertices will be pentagons, the rest are hexagons
        bool isPentagon = vertexIndex < 12;
        Tile::TileShape shape = isPentagon ? Tile::TileShape::Pentagon : Tile::TileShape::Hexagon;
          // Create a new tile centered at the vertex
        Tile tile(subdivisionVertices[vertexIndex], shape);
        
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
        tiles.push_back(std::move(tile));
        
        // Count pentagons
        if (isPentagon) pentagonCount++;
        
        // Report progress periodically
        processedVertices++;
        if (progressTracker && processedVertices % 200 == 0) {
            float progress = static_cast<float>(processedVertices) / totalVertices;
            std::string message = "Creating tiles (" + 
                                 std::to_string(processedVertices) + " of " + 
                                 std::to_string(totalVertices) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Debug check
    if (pentagonCount != 12) {
        std::cerr << "Warning: Expected 12 pentagons but got " << pentagonCount << std::endl;
    }
    
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Created " + std::to_string(tiles.size()) + " tiles");
    }
}

void World::SetupTileNeighbors() {
    // Build edge-to-tiles mapping during tile creation for O(E) complexity
    std::unordered_map<uint64_t, std::vector<int>> edgeToTiles;
    
    // For each tile, register all its edges
    for (size_t tileIdx = 0; tileIdx < tiles.size(); tileIdx++) {
        const auto& tile = tiles[tileIdx];
        const auto& vertices = tile.GetVertices();
        
        // For each edge in the tile (connecting consecutive vertices)
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t nextIdx = (i + 1) % vertices.size();
            
            // Create edge key using vertex hashes (ensure consistent ordering)
            size_t hash1 = std::hash<glm::vec3>{}(vertices[i]);
            size_t hash2 = std::hash<glm::vec3>{}(vertices[nextIdx]);
            uint64_t edgeKey = (hash1 < hash2) ? 
                               ((static_cast<uint64_t>(hash1) << 32) | hash2) :
                               ((static_cast<uint64_t>(hash2) << 32) | hash1);
            
            // Add this tile to the edge mapping
            edgeToTiles[edgeKey].push_back(static_cast<int>(tileIdx));
        }
        
        // Report progress periodically
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = static_cast<float>(tileIdx) / tiles.size() * 0.3f;
            std::string message = "Building edge mapping (" + 
                                 std::to_string(tileIdx) + " of " + 
                                 std::to_string(tiles.size()) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Now establish neighborhood relationships using the edge mapping
    for (size_t tileIdx = 0; tileIdx < tiles.size(); tileIdx++) {
        auto& tile = tiles[tileIdx];
        const auto& vertices = tile.GetVertices();
        std::vector<int> neighbors;
        
        // For each edge in the tile, find tiles that share this edge
        for (size_t i = 0; i < vertices.size(); i++) {
            size_t nextIdx = (i + 1) % vertices.size();
            
            // Create the same edge key
            size_t hash1 = std::hash<glm::vec3>{}(vertices[i]);
            size_t hash2 = std::hash<glm::vec3>{}(vertices[nextIdx]);
            uint64_t edgeKey = (hash1 < hash2) ? 
                               ((static_cast<uint64_t>(hash1) << 32) | hash2) :
                               ((static_cast<uint64_t>(hash2) << 32) | hash1);
            
            // Find all tiles sharing this edge
            auto it = edgeToTiles.find(edgeKey);
            if (it != edgeToTiles.end()) {
                for (int otherTileIdx : it->second) {
                    // Don't add ourselves as a neighbor
                    if (otherTileIdx != static_cast<int>(tileIdx)) {
                        neighbors.push_back(otherTileIdx);
                    }
                }
            }
        }
        
        // Remove duplicates (a tile might be added multiple times if sharing multiple edges)
        std::sort(neighbors.begin(), neighbors.end());
        neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
        
        // Set the neighbors
        tile.SetNeighbors(neighbors);
        
        // Report progress periodically
        if (progressTracker && tileIdx % 1000 == 0) {
            float progress = 0.3f + static_cast<float>(tileIdx) / tiles.size() * 0.7f;
            std::string message = "Establishing tile connections (" + 
                                 std::to_string(tileIdx) + " of " + 
                                 std::to_string(tiles.size()) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Report completion
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Completed neighborhood setup");
    }
}

void World::InitializeBaseTiles() {
    // Initialize all tiles with base values
    // The plate-based system will set the actual terrain
    
    // Initialize all tiles with default values
    // Plate assignment will determine ocean vs land
    for (size_t i = 0; i < tiles.size(); i++) {
        // Set neutral elevation - plate system will determine actual values
        tiles[i].SetElevation(0.5f);
        
        // Set default terrain type - will be updated by plate system
        tiles[i].SetTerrainType(TerrainType::Lowland);
        
        // Set neutral moisture - can be updated later by climate system
        tiles[i].SetMoisture(0.5f);
        
        // Temperature based on latitude (this can stay)
        glm::vec3 pos = tiles[i].GetCenter();
        float latitude = std::asin(pos.y);  // -π/2 to +π/2
        float normalizedLatitude = latitude / (3.14159f / 2.0f);  // -1 to +1
        float temperature = 0.8f - 0.6f * std::abs(normalizedLatitude);
        temperature = glm::clamp(temperature, 0.0f, 1.0f);
        tiles[i].SetTemperature(temperature);
        
        // Report progress periodically
        if (progressTracker && i % 1000 == 0) {
            float progress = static_cast<float>(i) / tiles.size();
            std::string message = "Initializing tiles (" + 
                                 std::to_string(i) + " of " + 
                                 std::to_string(tiles.size()) + ")";
            progressTracker->UpdateProgress(progress, message);
        }
    }
    
    // Report progress before smoothing starts
    if (progressTracker) {
        progressTracker->UpdateProgress(0.7f, "Smoothing terrain data...");
    }
    
    // Smooth the terrain properties by averaging with neighbors
    SmoothTerrainData();
    
    // Report completion
    if (progressTracker) {
        progressTracker->UpdateProgress(1.0f, "Terrain generation complete");
    }
}

void World::SmoothTerrainData() {
    // Create a copy of the current terrain data
    std::vector<float> oldElevations(tiles.size());
    std::vector<float> oldMoistures(tiles.size());
    std::vector<float> oldTemperatures(tiles.size());
    
    for (size_t i = 0; i < tiles.size(); i++) {
        oldElevations[i] = tiles[i].GetElevation();
        oldMoistures[i] = tiles[i].GetMoisture();
        oldTemperatures[i] = tiles[i].GetTemperature();
    }
    
    // Smooth the terrain data by averaging with neighbors
    for (size_t i = 0; i < tiles.size(); i++) {
        const auto& neighbors = tiles[i].GetNeighbors();
        if (neighbors.empty()) continue;
        
        // Calculate average values considering neighbors
        float sumElevation = oldElevations[i];
        float sumMoisture = oldMoistures[i];
        float sumTemperature = oldTemperatures[i];
        int count = 1;  // start with 1 for the tile itself
        
        for (int neighborIdx : neighbors) {
            if (neighborIdx >= 0 && neighborIdx < static_cast<int>(tiles.size())) {
                sumElevation += oldElevations[neighborIdx];
                sumMoisture += oldMoistures[neighborIdx];
                sumTemperature += oldTemperatures[neighborIdx];
                count++;
            }
        }
          // Apply the smoothed values
        float smoothedElevation = sumElevation / count;
        tiles[i].SetElevation(smoothedElevation);
        tiles[i].SetMoisture(sumMoisture / count);
        tiles[i].SetTemperature(sumTemperature / count);
        
        // Update terrain type based on smoothed elevation
        const float waterLevel = 0.4f;
        TerrainType terrainType;
        if (smoothedElevation < waterLevel - 0.2f) {
            terrainType = TerrainType::Ocean;
        } else if (smoothedElevation < waterLevel - 0.05f) {
            terrainType = TerrainType::Shallow;
        } else if (smoothedElevation < waterLevel + 0.05f) {
            terrainType = TerrainType::Beach;
        } else if (smoothedElevation < waterLevel + 0.3f) {
            terrainType = TerrainType::Lowland;
        } else if (smoothedElevation < waterLevel + 0.6f) {
            terrainType = TerrainType::Highland;
        } else if (smoothedElevation < waterLevel + 0.8f) {
            terrainType = TerrainType::Mountain;
        } else {
            terrainType = TerrainType::Peak;
        }
        
        // Set the terrain type
        tiles[i].SetTerrainType(terrainType);
    }
}

int World::FindTileContainingPoint(const glm::vec3& point, int previousTileIndex) const {
    // Normalize the point to ensure it's on the unit sphere
    glm::vec3 normalizedPoint = glm::normalize(point);
    
    // If no previous tile provided, fall back to global search
    if (previousTileIndex < 0 || previousTileIndex >= tiles.size()) {
        return Core::findNearestTile(normalizedPoint, tiles);
    }
    
    // LOCAL SEARCH OPTIMIZATION:
    // Since chunks are small relative to world tiles, sequential sample points
    // are likely to be in the same tile or an immediate neighbor.
    // This reduces search from O(80,000) to O(6-12) tiles.
    
    // First, check if the point is still in the previous tile
    if (isPointInTile(normalizedPoint, previousTileIndex)) {
        return previousTileIndex;
    }
    
    // Search the immediate neighbors of the previous tile (typically 5-6 tiles)
    const auto& neighbors = tiles[previousTileIndex].GetNeighbors();
    for (int neighborIdx : neighbors) {
        if (isPointInTile(normalizedPoint, neighborIdx)) {
            return neighborIdx;
        }
    }
    
    // If we're sampling at chunk boundaries, we might need to search 2 tiles away
    // This tracks which tiles we've already checked to avoid duplicates
    std::unordered_set<int> searched;
    searched.insert(previousTileIndex);
    for (int neighborIdx : neighbors) {
        searched.insert(neighborIdx);
    }
    
    // Search neighbors-of-neighbors (2 hops from previous tile)
    for (int neighborIdx : neighbors) {
        const auto& secondNeighbors = tiles[neighborIdx].GetNeighbors();
        for (int secondNeighborIdx : secondNeighbors) {
            if (searched.find(secondNeighborIdx) == searched.end()) {
                if (isPointInTile(normalizedPoint, secondNeighborIdx)) {
                    return secondNeighborIdx;
                }
                searched.insert(secondNeighborIdx);
            }
        }
    }
    
    // If local search fails (shouldn't happen with proper chunk sizes),
    // fall back to global search
    std::cerr << "WARNING: Local tile search failed, falling back to global search" << std::endl;
    return Core::findNearestTile(normalizedPoint, tiles);
}

bool World::isPointInTile(const glm::vec3& point, int tileIndex) const {
    // Simple Voronoi cell test: A point belongs to a tile if that tile's
    // center is closer than any neighboring tile's center.
    // This works because our tiles form a Voronoi diagram on the sphere.
    
    const auto& tileCenter = tiles[tileIndex].GetCenter();
    float distToCenter = glm::distance2(point, tileCenter);  // squared distance for efficiency
    
    // Check if any neighbor is closer
    const auto& neighbors = tiles[tileIndex].GetNeighbors();
    for (int neighborIdx : neighbors) {
        float distToNeighbor = glm::distance2(point, tiles[neighborIdx].GetCenter());
        if (distToNeighbor < distToCenter) {
            return false;  // Neighbor is closer, point belongs to neighbor
        }
    }
    
    return true;  // This tile's center is closest, point belongs here
}

void World::SetPlates(const std::vector<Plate>& plates) {
    tectonicPlates = plates;
}

} // namespace Generators
} // namespace WorldGen