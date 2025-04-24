#define _USE_MATH_DEFINES
#include <cmath>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "PlateGenerator.h"
#include "Lithosphere.h" // Include Lithosphere
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <algorithm>
#include <queue>

namespace WorldGen {

PlateGenerator::PlateGenerator(const PlanetParameters& parameters, uint64_t seed)
{
    // No longer needs parameters or random directly
    // Create the Lithosphere instance
    m_lithosphere = std::make_unique<Lithosphere>(parameters, seed);
}

} // namespace WorldGen