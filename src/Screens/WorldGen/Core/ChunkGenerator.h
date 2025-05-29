#pragma once

#include <memory>
#include <glm/glm.hpp>
#include "ChunkTypes.h"
#include "../Generators/World.h"

namespace WorldGen {
namespace Core {

/**
 * @brief Utility class for generating terrain chunks from spherical world data.
 * 
 * DESIGN RATIONALE:
 * This class encapsulates the logic for sampling a 3D spherical world and
 * projecting it onto 2D chunks for gameplay. Each chunk is generated independently
 * based on its position on the sphere, allowing for:
 * 
 * - On-demand generation as players explore
 * - Parallel generation of multiple chunks
 * - Consistent results regardless of generation order
 * - Support for multiple play areas on the same globe
 */
class ChunkGenerator {
public:
    /**
     * @brief Generate a chunk at the specified location on the sphere.
     * 
     * This function:
     * 1. Creates a local tangent plane at the chunk center
     * 2. Projects a grid of points onto the sphere
     * 3. Samples the nearest world tile for each point
     * 4. Converts the 3D tile data to 2D terrain data
     * 5. Calculates game positions for each tile relative to world origin (prime meridian/equator)
     * 
     * @param worldGenerator The spherical world to sample from
     * @param chunkCenter Center position of the chunk on the unit sphere
     * @return Generated chunk data with tiles filled in with world-space coordinates
     */
    static std::unique_ptr<ChunkData> generateChunk(
        const WorldGen::Generators::World& worldGenerator,
        const glm::vec3& chunkCenter
    );
    
    /**
     * @brief Create a local tangent basis for projection at a point on the sphere.
     * 
     * The basis consists of three orthonormal vectors:
     * - East: Points along lines of longitude
     * - North: Points toward the north pole
     * - Up: Points away from sphere center (normal to surface)
     * 
     * @param pointOnSphere Point on the unit sphere
     * @return 3x3 matrix with columns [east, north, up]
     */
    static glm::mat3 createLocalTangentBasis(const glm::vec3& pointOnSphere);
    
    /**
     * @brief Project a local 2D point to the sphere surface.
     * 
     * Takes a point in the local chunk coordinate system and projects it
     * onto the sphere. The projection accounts for the curvature of the
     * sphere to minimize distortion.
     * 
     * @param localPoint 2D point in chunk space (meters from chunk center)
     * @param chunkCenter Center of chunk on unit sphere
     * @param tangentBasis Local tangent plane basis
     * @return 3D point on the unit sphere
     */
    static glm::vec3 projectToSphere(
        const glm::vec2& localPoint,
        const glm::vec3& chunkCenter,
        const glm::mat3& tangentBasis
    );
};

} // namespace Core
} // namespace WorldGen