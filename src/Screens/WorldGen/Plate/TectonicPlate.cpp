#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "TectonicPlate.h"
#include <glm/gtx/rotate_vector.hpp>

namespace WorldGen {

TectonicPlate::TectonicPlate(int id, PlateType type, const glm::vec3& center)
    : m_id(id)
    , m_type(type)
    , m_center(center)
    , m_movementVector(0.0f)
    , m_rotationRate(0.0f)
{
}

glm::vec3 TectonicPlate::CalculateMovementAt(const glm::vec3& position) const {
    // Calculate the vector from center to position
    glm::vec3 toPosition = position - m_center;
    float distance = glm::length(toPosition);
    
    if (distance < 0.0001f) {
        return m_movementVector;
    }
    
    // Normalize the vector
    glm::vec3 direction = toPosition / distance;
    
    // Calculate rotation at this point
    glm::vec3 rotationAxis = glm::cross(m_center, position);
    if (glm::length(rotationAxis) > 0.0001f) {
        rotationAxis = glm::normalize(rotationAxis);
        glm::vec3 rotationMovement = m_rotationRate * glm::cross(rotationAxis, direction);
        return m_movementVector + rotationMovement;
    }
    
    return m_movementVector;
}

float TectonicPlate::GetBaseElevation() const {
    // Continental plates have higher base elevation than oceanic plates
    return m_type == PlateType::Continental ? 0.2f : -0.2f;
}

} // namespace WorldGen 