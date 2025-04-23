#include "WorldGen/Core/TectonicPlates/TectonicPlate.h"
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace WorldGen {
namespace Core {

TectonicPlate::TectonicPlate(int id, PlateType type, const glm::vec3& center)
    : m_id(id)
    , m_type(type)
    , m_center(center)
    , m_rotationRate(0.0f)
{
    // Initialize with zero movement vector
    m_movementVector = glm::vec3(0.0f);
}

glm::vec3 TectonicPlate::CalculateMovementAt(const glm::vec3& position) const
{
    // Calculate linear movement component
    glm::vec3 movement = m_movementVector;
    
    // If there's rotation, add rotational component
    if (m_rotationRate != 0.0f) {
        // Calculate vector from center to position
        glm::vec3 toPosition = position - m_center;
        
        // Normalize to get direction
        glm::vec3 direction = glm::normalize(toPosition);
        
        // Calculate rotation axis (perpendicular to both center and position)
        glm::vec3 rotationAxis = glm::normalize(glm::cross(m_center, position));
        
        // Calculate tangential velocity based on rotation rate and distance from rotation axis
        float distance = glm::length(toPosition);
        float tangentialSpeed = distance * m_rotationRate;
        
        // Calculate tangential direction (perpendicular to both rotation axis and direction to position)
        glm::vec3 tangentialDirection = glm::normalize(glm::cross(rotationAxis, direction));
        
        // Add rotational component to movement
        movement += tangentialDirection * tangentialSpeed;
    }
    
    return movement;
}

float TectonicPlate::GetBaseElevation() const
{
    // Continental plates have higher base elevation than oceanic plates
    return (m_type == PlateType::Continental) ? 0.2f : -0.2f;
}

} // namespace Core
} // namespace WorldGen
