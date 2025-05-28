#pragma once

#include <unordered_map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "TerrainTypes.h"

namespace WorldGen {
namespace Core {

/**
 * @brief Chunk coordinate system based on spherical position.
 * 
 * DESIGN DECISION: Chunks are indexed by their center position on the unit sphere.
 * This allows chunks to be generated independently at any location on the globe,
 * supporting features like:
 * - Teleportation to distant locations
 * - Multiple simultaneous play areas
 * - Consistent chunk generation regardless of access pattern
 * 
 * Each chunk represents a small "patch" of the sphere's surface, projected onto
 * a local tangent plane for 2D gameplay. The size of this patch is determined
 * by the angular extent needed to cover the desired area in meters/kilometers.
 */
struct ChunkCoord {
    glm::vec3 centerOnSphere;  // Normalized 3D point on unit sphere (the chunk's center)
    
    // Constructor
    ChunkCoord() : centerOnSphere(0, 1, 0) {}  // Default to north pole
    explicit ChunkCoord(const glm::vec3& center) : centerOnSphere(glm::normalize(center)) {}
    
    bool operator==(const ChunkCoord& other) const {
        // CRITICAL: Chunk equality comparison precision issue
        // 
        // Using dot product for comparison has precision issues:
        // - Adjacent 400m chunks can be incorrectly considered equal
        // - Dot product of normalized vectors sometimes exceeds 1.0 due to float precision
        // - This causes the multi-chunk system to fail - chunks load but are considered
        //   the same chunk, so tiles from adjacent chunks cannot be found
        //
        // Solution: Use squared distance with a very small threshold
        // This avoids the numerical instability of dot product near 1.0
        
        // TODO: This threshold should be dynamically calculated based on chunk size
        // For now, use a threshold that's smaller than the minimum chunk spacing
        // For 400m chunks on sphere radius 1: distance ≈ 2*sin(200m/6371000m) ≈ 0.0000628
        const float distanceSquaredThreshold = 0.000001f * 0.000001f;  // ~6m on unit sphere
        
        glm::vec3 diff = centerOnSphere - other.centerOnSphere;
        return glm::dot(diff, diff) < distanceSquaredThreshold;
    }
};

/**
 * @brief Data for a single chunk of terrain.
 * 
 * Each chunk contains a 2D grid of tiles that represent the terrain
 * in a local area around the chunk's center point on the sphere.
 */
struct ChunkData {
    ChunkCoord coord;                                    // Position on sphere
    glm::mat3 localTangentBasis;                        // Basis vectors for local projection
    std::unordered_map<TileCoord, TerrainData> tiles;  // Local tile data
    bool isLoaded = false;
    bool isGenerating = false;
    float lastAccessTime = 0.0f;
};

/**
 * @brief Calculate the angular size of a chunk on the sphere.
 * 
 * Given the desired chunk size in meters and the planet radius,
 * calculate the angular extent in radians.
 * 
 * @param chunkSizeMeters Size of chunk edge in meters
 * @param planetRadius Radius of the planet in meters (default Earth: 6,371,000m)
 * @return Angular size in radians
 */
inline float getChunkAngularSize(float chunkSizeMeters, float planetRadius = 6371000.0f) {
    return chunkSizeMeters / planetRadius;
}

/**
 * @brief Calculate neighboring chunk centers on the sphere.
 * 
 * Given a chunk center and the angular size of chunks, calculate the centers
 * of the 8 neighboring chunks (N, NE, E, SE, S, SW, W, NW).
 * 
 * @param center Current chunk center on unit sphere
 * @param angularSize Angular size of chunk in radians
 * @return Vector of 8 neighboring chunk centers
 */
std::vector<glm::vec3> getNeighboringChunkCenters(const glm::vec3& center, float angularSize);

} // namespace Core
} // namespace WorldGen

// Hash function for ChunkCoord to use in unordered_map
namespace std {
    template<>
    struct hash<WorldGen::Core::ChunkCoord> {
        size_t operator()(const WorldGen::Core::ChunkCoord& coord) const {
            // Hash based on the spherical position
            // We'll use a spatial hash that's stable for nearby positions
            
            // Convert to spherical coordinates for stable hashing
            float theta = atan2(coord.centerOnSphere.z, coord.centerOnSphere.x);
            float phi = asin(coord.centerOnSphere.y);
            
            // Quantize to ensure nearby positions hash the same
            const float quantization = 0.001f;  // ~100m resolution on Earth
            int thetaQuant = static_cast<int>(theta / quantization);
            int phiQuant = static_cast<int>(phi / quantization);
            
            // Combine hashes
            size_t h1 = std::hash<int>()(thetaQuant);
            size_t h2 = std::hash<int>()(phiQuant);
            return h1 ^ (h2 << 1);
        }
    };
}