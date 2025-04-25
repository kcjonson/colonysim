#define GLM_ENABLE_EXPERIMENTAL
#include "Lithosphere.h"
#include <glm/gtx/vector_angle.hpp> // For glm::angle
#include <glm/gtx/norm.hpp> // For glm::length
#include <algorithm> // For std::find_if
#include <iostream> // For debug output
#include <limits> // Required for std::numeric_limits
#include <cmath> // Required for std::sqrt, std::uniform_real_distribution, std::acos, std::abs
#include <map> // Use map for ordered boundary processing
#include <unordered_map> // For temporary storage in ModifyCrust
#include <unordered_set>
#include <set> // For ordered set to avoid duplicate edges/vertices and processed boundaries
#include <glm/gtx/rotate_vector.hpp> // For glm::rotate
#include <glm/gtc/quaternion.hpp> // For quaternions
#include <glm/gtc/constants.hpp> // For pi()


namespace WorldGen {

Lithosphere::Lithosphere(const PlanetParameters& parameters, uint64_t seed)
    : m_parameters(parameters)
    , m_random(seed)
{
    std::cout << "Lithosphere created with seed: " << seed << std::endl;
}


void Lithosphere::Update(float deltaTime, const std::vector<glm::vec3>& planetVertices, const std::vector<unsigned int>& planetIndices) {
    // std::cout << "Lithosphere::Update called with deltaTime: " << deltaTime << std::endl;

    // 1. Move plates based on their velocity and rotation
    MovePlates(deltaTime);

    // 2. Re-assign vertices to the plates based on new centers
    AssignVerticesToPlates(planetVertices);

    // 3. Re-detect boundaries based on the new vertex assignments
    DetectBoundaries(planetVertices, planetIndices);

    // 4. Analyze boundaries (Determine type, calculate stress)
    AnalyzeBoundaries(planetVertices); // Pass vertices needed for calculations

    // 5. Modify crust based on boundary interactions (subduction, uplift, etc.)
    ModifyCrust(deltaTime); // Pass deltaTime

    // 6. Recalculate plate masses based on potentially changed crust thickness
    RecalculatePlateMasses();
}

const std::vector<std::shared_ptr<TectonicPlate>>& Lithosphere::GetPlates() const {
    return m_plates;
}

std::vector<std::shared_ptr<TectonicPlate>>& Lithosphere::GetPlates() {
    return m_plates;
}


// Helper function to get a plate by its ID
TectonicPlate* Lithosphere::GetPlateById(int id) {
    auto it = std::find_if(m_plates.begin(), m_plates.end(),
                           [id](const std::shared_ptr<TectonicPlate>& plate) {
                               // Use ->get() to access the raw pointer if needed, or just compare IDs
                               return plate->GetId() == id;
                           });
    // Return raw pointer using ->get()
    return (it != m_plates.end()) ? it->get() : nullptr;
}

} // namespace WorldGen
