#define GLM_ENABLE_EXPERIMENTAL
#include "Lithosphere.h"
#include <glm/gtx/vector_angle.hpp> // For glm::angle
#include <glm/gtx/norm.hpp> // For glm::length
#include <algorithm> // For std::find_if, std::sort, std::unique, std::min/max
#include <iostream> // For debug output
#include <limits> // Required for std::numeric_limits
#include <cmath> // Required for std::sqrt, std::uniform_real_distribution, std::acos, std::abs
#include <map> // Use map for ordered boundary processing
#include <unordered_map> // For temporary storage in ModifyCrust
#include <set> // For ordered set to avoid duplicate edges/vertices and processed boundaries
#include <glm/gtx/rotate_vector.hpp> // For glm::rotate
#include <glm/gtc/quaternion.hpp> // For quaternions


namespace WorldGen {

void Lithosphere::MovePlates(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    for (auto& plate : m_plates) {
        glm::vec3 currentCenter = plate->GetCenter();
        glm::vec3 movementVector = plate->GetMovementVector(); // Linear velocity tangent to sphere
        float rotationRate = plate->GetRotationRate(); // Angular velocity around plate center

        // 1. Apply linear movement (translate center along great circle)
        if (glm::length(movementVector) > 0.0001f) {
            float distance = glm::length(movementVector) * deltaTime;
            glm::vec3 axis = glm::normalize(glm::cross(currentCenter, movementVector));
            currentCenter = glm::rotate(currentCenter, distance, axis);
            plate->SetCenter(currentCenter); // Update the plate's center
        }

        // 2. Apply rotation around the plate's center
        // This rotation affects the orientation of the plate, which influences boundary interactions
        // and how vertices might be assigned if using a more sophisticated method later.
        // For now, we just update the plate's internal rotation state if needed.
        // The actual effect on vertices happens during re-assignment based on the *new* center.
        if (std::abs(rotationRate) > 0.0001f) {
            float angle = rotationRate * deltaTime;
            glm::vec3 rotationAxis = plate->GetCenter(); // Rotate around axis passing through center

            // We need to update the plate's internal representation of its orientation.
            // Let's assume TectonicPlate stores a rotation quaternion or similar.
            // For now, we'll just conceptually note that the plate rotates.
            // plate->ApplyRotation(rotationAxis, angle); // Hypothetical method

            // Also, update the movement vector to reflect the rotation of the plate itself
            movementVector = glm::rotate(movementVector, angle, rotationAxis);
            plate->SetMovementVector(movementVector);
        }
    }
    // std::cout << "Moved plates for deltaTime: " << deltaTime << std::endl;
}

void Lithosphere::ModifyCrust(float deltaTime) {
    // std::cout << "Lithosphere::ModifyCrust called." << std::endl; // Less verbose
    if (m_plates.empty() || deltaTime <= 0.0f) return;

    // Constants for crust modification (tune these values)
    const float subductionRate = 0.1f * deltaTime; // Rate at which oceanic crust subducts
    const float orogenyRate = 0.05f * deltaTime;   // Rate at which continental crust thickens
    const float riftingRate = 0.02f * deltaTime;   // Rate at which crust thins at divergent boundaries
    const float ageIncreaseRate = 1.0f * deltaTime; // Rate at which crust ages (arbitrary units)
    const float minThickness = 0.01f; // Minimum crust thickness
    const float maxThickness = 2.0f;  // Maximum crust thickness

    // Store changes temporarily to avoid race conditions if modifying shared vertices
    std::unordered_map<int, float> thicknessChanges; // vertexIndex -> change in thickness
    std::unordered_map<int, float> ageChanges;       // vertexIndex -> new age (absolute value)

    // Iterate through boundaries to apply modifications
    std::set<std::pair<int, int>> processedBoundaries; // Track processed boundary pairs

    for (auto& plate1_ptr : m_plates) {
        TectonicPlate& plate1 = *plate1_ptr;
        for (const auto& boundary : plate1.GetBoundaries()) {
            int plate1Id = plate1.GetId();
            int plate2Id = (boundary.plate1Index == plate1Id) ? boundary.plate2Index : boundary.plate1Index;

            // Ensure we process each boundary pair only once
            std::pair<int, int> boundaryKey = {std::min(plate1Id, plate2Id), std::max(plate1Id, plate2Id)};
            if (processedBoundaries.count(boundaryKey)) continue;
            processedBoundaries.insert(boundaryKey);

            TectonicPlate* plate2_ptr = GetPlateById(plate2Id);
            if (!plate2_ptr) continue;
            TectonicPlate& plate2 = *plate2_ptr;

            float convergenceSpeed = boundary.m_convergenceSpeed; // Positive for convergence, negative for divergence
            float stress = boundary.stress;

            // Apply effects based on boundary type
            switch (boundary.type) {
                case BoundaryType::Convergent: { // Add opening brace for case scope
                    // Determine which plate subducts (oceanic under continental, older oceanic under younger oceanic)
                    TectonicPlate* subductingPlate = nullptr;
                    TectonicPlate* overridingPlate = nullptr;
                    bool continentalCollision = false;

                    if (plate1.GetType() == PlateType::Oceanic && plate2.GetType() == PlateType::Continental) {
                        subductingPlate = &plate1; overridingPlate = &plate2;
                    } else if (plate1.GetType() == PlateType::Continental && plate2.GetType() == PlateType::Oceanic) {
                        subductingPlate = &plate2; overridingPlate = &plate1;
                    } else if (plate1.GetType() == PlateType::Oceanic && plate2.GetType() == PlateType::Oceanic) {
                        // Simplistic: Assume older plate subducts (needs average age near boundary)
                        // For now, just pick one based on ID for determinism
                        // TODO: Implement age-based subduction for O-O
                        if (plate1Id < plate2Id) { // Arbitrary choice for now
                             subductingPlate = &plate1; overridingPlate = &plate2;
                        } else {
                             subductingPlate = &plate2; overridingPlate = &plate1;
                        } // <<< Added missing closing brace here
                    } else { // Continental-Continental
                        continentalCollision = true;
                        // Both plates contribute to orogeny
                    }

                    // Apply effects to vertices near the boundary
                    for (int vertexIndex : boundary.m_sharedVertexIndices) {
                        if (continentalCollision) {
                            // Orogeny: Thicken crust on both plates at the boundary
                            float thicknessIncrease = orogenyRate * std::abs(convergenceSpeed) * stress * 0.5f; // Split effect
                            thicknessChanges[vertexIndex] += thicknessIncrease; // Accumulate changes
                            ageChanges[vertexIndex] = 0.0f; // Reset age due to mountain building (simplification)
                        } else {
                            // Subduction/Orogeny involving at least one oceanic plate
                            if (overridingPlate) {
                                // Thicken overriding plate's crust (orogeny/volcanic arc)
                                float thicknessIncrease = orogenyRate * std::abs(convergenceSpeed) * stress;
                                thicknessChanges[vertexIndex] += thicknessIncrease;
                                ageChanges[vertexIndex] = 0.0f; // Reset age due to uplift/volcanism
                            }
                            if (subductingPlate && subductingPlate->GetType() == PlateType::Oceanic) {
                                // Subduction: Thin/destroy subducting oceanic crust
                                float thicknessDecrease = -subductionRate * std::abs(convergenceSpeed);
                                thicknessChanges[vertexIndex] += thicknessDecrease;
                                // Age becomes irrelevant as it's destroyed/recycled
                                ageChanges[vertexIndex] = 0.0f; // Reset age
                            }
                        }
                    }
                    break;
                } // Add closing brace for case scope
                case BoundaryType::Divergent: { // Add opening brace for case scope
                    // Rifting: Thin crust and create new young crust
                    for (int vertexIndex : boundary.m_sharedVertexIndices) {
                        float thicknessChange = -riftingRate * std::abs(convergenceSpeed); // Thinning
                        thicknessChanges[vertexIndex] += thicknessChange;
                        ageChanges[vertexIndex] = 0.0f; // New crust is young
                    }
                    break;
                } // Add closing brace for case scope
                case BoundaryType::Transform: { // Add opening brace for case scope
                    // Minimal crust modification, maybe some minor stress-related effects later
                    // For now, just age the crust normally (handled in the final loop)
                    break;
                } // Add closing brace for case scope
            }
        }
    }

    // Apply accumulated changes and general aging to all vertices
    for (auto& plate : m_plates) {
        // std::vector<int> verticesToRemove; // If subducted crust gets removed entirely
        auto& thicknessMap = plate->GetVertexCrustThickness();
        auto& ageMap = plate->GetVertexCrustAge();
        const auto& vertexIndices = plate->GetVertexIndices(); // Get indices for this plate

        for (int vertexIndex : vertexIndices) {
            float currentThickness = thicknessMap[vertexIndex];
            float currentAge = ageMap[vertexIndex];

            // Apply specific boundary changes
            if (thicknessChanges.count(vertexIndex)) {
                currentThickness += thicknessChanges[vertexIndex];
            }
            if (ageChanges.count(vertexIndex)) {
                currentAge = ageChanges[vertexIndex]; // Set new absolute age
            } else {
                // Apply general aging if no boundary interaction reset it
                currentAge += ageIncreaseRate;
            }

            // Clamp thickness
            thicknessMap[vertexIndex] = glm::clamp(currentThickness, minThickness, maxThickness);
            ageMap[vertexIndex] = currentAge;

            // Check for crust removal (e.g., fully subducted) - More complex logic needed
            // if (thicknessMap[vertexIndex] <= minThickness && plate->GetType() == PlateType::Oceanic) {
            //     // Mark vertex for removal or transfer? Requires careful handling of mesh data.
            // }
        }

        // Process vertex removals/transfers here if implemented
    }
     // std::cout << "Finished modifying crust." << std::endl; // Less verbose
}


void Lithosphere::RecalculatePlateMasses() {
    // std::cout << "Lithosphere::RecalculatePlateMasses called." << std::endl; // Less verbose
    for (auto& plate : m_plates) {
        float plateMass = 0.0f;
        const auto& thicknessMap = plate->GetVertexCrustThickness();
        const auto& vertexIndices = plate->GetVertexIndices();

        if (vertexIndices.empty()) {
            plate->SetTotalMass(0.0f);
            continue;
        }

        for (int vertexIndex : vertexIndices) {
            // Simple mass calculation: sum of thickness
            // TODO: Improve by considering vertex area and density
            auto it = thicknessMap.find(vertexIndex);
            if (it != thicknessMap.end()) {
                 plateMass += it->second;
            } else {
                // This shouldn't happen if initialization and modification are correct
                std::cerr << "Warning: Vertex " << vertexIndex << " on plate " << plate->GetId() << " missing thickness during mass recalc." << std::endl;
                // Assign a default thickness based on plate type? Or 0?
                plateMass += (plate->GetType() == PlateType::Continental) ? 0.5f : 0.2f;
            }
        }
        plate->SetTotalMass(plateMass);
    }
    // std::cout << "Finished recalculating plate masses." << std::endl; // Less verbose
}


} // namespace WorldGen
