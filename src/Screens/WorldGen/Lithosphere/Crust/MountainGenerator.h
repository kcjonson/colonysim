/*
 * DEPRECATED - This class-based mountain generator is no longer used.
 * The functional plate generation system handles mountain formation differently.
 * This file is kept for reference but should not be used in new code.
 */
#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../../Core/TerrainTypes.h"

namespace WorldGen {

// Forward declarations
class TectonicPlate;

class MountainGenerator {
public:
    MountainGenerator();
    ~MountainGenerator() = default;
    
    // Calculate elevation with mountain features based on plate interactions
    float CalculateElevationWithMountains(
        const glm::vec3& point,
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int plateIndex,
        float baseElevation,
        const std::vector<glm::vec3>& planetVertices);
        
    // Generate mountain ranges along plate boundaries
    void GenerateMountainRanges(
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        const std::vector<glm::vec3>& planetVertices);
        
private:
    // Calculate mountain height based on boundary type and stress
    float CalculateMountainHeight(float boundaryStress, float convergenceAngle);
    
    // Calculate distance from point to nearest plate boundary
    float DistanceToBoundary(
        const glm::vec3& point,
        const std::vector<std::shared_ptr<TectonicPlate>>& plates,
        int plateIndex,
        const std::vector<glm::vec3>& planetVertices);
};

} // namespace WorldGen