#define _USE_MATH_DEFINES
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "PlateGenerator.h"
#include "../Lithosphere.h" // Updated path to Lithosphere.h
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <queue>
#include <limits>

namespace WorldGen {

PlateGenerator::PlateGenerator(const PlanetParameters& parameters, uint64_t seed)
{
    // No longer needs parameters or random directly
    // Create the Lithosphere instance
    m_lithosphere = std::make_unique<Lithosphere>(parameters, seed);
    
    // Create the MountainGenerator instance
    m_mountainGenerator = std::make_unique<MountainGenerator>();
}

float PlateGenerator::CalculateElevationAtPoint(
    const glm::vec3& point,
    const std::vector<std::shared_ptr<TectonicPlate>>& plates,
    const std::vector<glm::vec3>& planetVertices)
{
    // Find which plate this point belongs to
    int plateIndex = -1;
    float minDistance = std::numeric_limits<float>::max();
    
    for (int i = 0; i < plates.size(); ++i) {
        float distance = glm::angle(point, plates[i]->GetCenter());
        if (distance < minDistance) {
            minDistance = distance;
            plateIndex = i;
        }
    }
    
    if (plateIndex == -1) {
        return 0.0f; // Fallback
    }
    
    // Get base elevation from plate type
    float baseElevation = plates[plateIndex]->GetBaseElevation();
    
    // Use the mountain generator to calculate elevation with mountains
    return m_mountainGenerator->CalculateElevationWithMountains(
        point, 
        plates, 
        plateIndex, 
        baseElevation,
        planetVertices
    );
}

} // namespace WorldGen