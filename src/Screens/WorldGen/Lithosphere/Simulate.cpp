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

bool Lithosphere::MovePlates(float deltaTime) {
    if (deltaTime <= 0.0f) return false;

    bool anyPlatesMoved = false;
    // Define thresholds for significant movement - increase these to reduce false positives
    const float MIN_POSITION_CHANGE = 0.001f;  // Increased from 0.0001f
    const float MIN_ANGLE_CHANGE = 0.001f;     // Increased from 0.0001f
    const float MIN_MOVEMENT_MAGNITUDE = 0.001f; // New threshold for movement vector magnitude

    for (auto& plate : m_plates) {
        glm::vec3 currentCenter = plate->GetCenter();
        glm::vec3 movementVector = plate->GetMovementVector(); // Linear velocity tangent to sphere
        float rotationRate = plate->GetRotationRate(); // Angular velocity around plate center

        // 1. Apply linear movement (translate center along great circle)
        bool positionChanged = false;
        if (glm::length(movementVector) > MIN_MOVEMENT_MAGNITUDE) {
            float distance = glm::length(movementVector) * deltaTime;
            // Only count as moved if the distance is significant
            if (distance > MIN_POSITION_CHANGE) {
                glm::vec3 axis = glm::normalize(glm::cross(currentCenter, movementVector));
                glm::vec3 newCenter = glm::rotate(currentCenter, distance, axis);
                
                // Only update if the change is significant - use distance squared for efficiency
                float distanceSquared = glm::distance2(newCenter, currentCenter);
                if (distanceSquared > MIN_POSITION_CHANGE * MIN_POSITION_CHANGE) {
                    plate->SetCenter(newCenter); // Update the plate's center
                    positionChanged = true;
                    anyPlatesMoved = true;
                }
            }
        }

        // 2. Apply rotation around the plate's center
        bool rotationChanged = false;
        if (std::abs(rotationRate) > MIN_ANGLE_CHANGE) {  // Only process if rotation rate is significant
            float angle = rotationRate * deltaTime;
            // Only count as rotated if the angle change is significant
            if (std::abs(angle) > MIN_ANGLE_CHANGE) {
                glm::vec3 rotationAxis = plate->GetCenter(); // Rotate around axis passing through center

                // Update the movement vector to reflect the rotation of the plate itself
                glm::vec3 newMovementVector = glm::rotate(movementVector, angle, rotationAxis);
                
                // Only update if the movement vector changed significantly - use distance squared
                float movementChangeSquared = glm::distance2(newMovementVector, movementVector);
                if (movementChangeSquared > MIN_POSITION_CHANGE * MIN_POSITION_CHANGE) {
                    plate->SetMovementVector(newMovementVector);
                    rotationChanged = true;
                    anyPlatesMoved = true;
                }
            }
        }
    }
    
    return anyPlatesMoved;
}

bool Lithosphere::ModifyCrust(float deltaTime) {
    if (m_plates.empty() || deltaTime <= 0.0f) return false;

    bool anyCrustModified = false;
    
    // Constants for crust modification thresholds
    const float SIGNIFICANT_THICKNESS_CHANGE = 0.01f; // Only count thickness changes above this threshold
    const float SIGNIFICANT_AGE_CHANGE = 5.0f;        // Only count age changes above this threshold

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

            // Skip boundaries with negligible effects
            if (std::abs(convergenceSpeed) < 0.001f || stress < 0.001f) continue;

            // Apply effects based on boundary type
            switch (boundary.type) {
                case BoundaryType::Convergent: {
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
                        if (plate1Id < plate2Id) {
                             subductingPlate = &plate1; overridingPlate = &plate2;
                        } else {
                             subductingPlate = &plate2; overridingPlate = &plate1;
                        }
                    } else { // Continental-Continental
                        continentalCollision = true;
                    }

                    // Apply effects to vertices near the boundary
                    for (int vertexIndex : boundary.m_sharedVertexIndices) {
                        if (continentalCollision) {
                            // Orogeny: Thicken crust on both plates at the boundary
                            float thicknessIncrease = orogenyRate * std::abs(convergenceSpeed) * stress * 0.5f; // Split effect
                            
                            // Only record significant changes
                            if (std::abs(thicknessIncrease) > SIGNIFICANT_THICKNESS_CHANGE) {
                                thicknessChanges[vertexIndex] += thicknessIncrease; // Accumulate changes
                                ageChanges[vertexIndex] = 0.0f; // Reset age due to mountain building
                            }
                        } else {
                            // Subduction/Orogeny involving at least one oceanic plate
                            if (overridingPlate) {
                                // Thicken overriding plate's crust (orogeny/volcanic arc)
                                float thicknessIncrease = orogenyRate * std::abs(convergenceSpeed) * stress;
                                
                                // Only record significant changes
                                if (std::abs(thicknessIncrease) > SIGNIFICANT_THICKNESS_CHANGE) {
                                    thicknessChanges[vertexIndex] += thicknessIncrease;
                                    ageChanges[vertexIndex] = 0.0f; // Reset age due to uplift/volcanism
                                }
                            }
                            if (subductingPlate && subductingPlate->GetType() == PlateType::Oceanic) {
                                // Subduction: Thin/destroy subducting oceanic crust
                                float thicknessDecrease = -subductionRate * std::abs(convergenceSpeed);
                                
                                // Only record significant changes
                                if (std::abs(thicknessDecrease) > SIGNIFICANT_THICKNESS_CHANGE) {
                                    thicknessChanges[vertexIndex] += thicknessDecrease;
                                    ageChanges[vertexIndex] = 0.0f; // Reset age
                                }
                            }
                        }
                    }
                    break;
                }
                case BoundaryType::Divergent: {
                    // Rifting: Thin crust and create new young crust
                    for (int vertexIndex : boundary.m_sharedVertexIndices) {
                        float thicknessChange = -riftingRate * std::abs(convergenceSpeed); // Thinning
                        
                        // Only record significant changes
                        if (std::abs(thicknessChange) > SIGNIFICANT_THICKNESS_CHANGE) {
                            thicknessChanges[vertexIndex] += thicknessChange;
                            ageChanges[vertexIndex] = 0.0f; // New crust is young
                        }
                    }
                    break;
                }
                case BoundaryType::Transform: {
                    // Minimal crust modification, no changes recorded
                    break;
                }
            }
        }
    }

    // Apply accumulated changes and general aging to all vertices
    for (auto& plate : m_plates) {
        auto& thicknessMap = plate->GetVertexCrustThickness();
        auto& ageMap = plate->GetVertexCrustAge();
        const auto& vertexIndices = plate->GetVertexIndices(); // Get indices for this plate

        for (int vertexIndex : vertexIndices) {
            float currentThickness = thicknessMap[vertexIndex];
            float currentAge = ageMap[vertexIndex];
            float newThickness = currentThickness;
            float newAge = currentAge;
            bool vertexModified = false;

            // Apply specific boundary changes
            if (thicknessChanges.count(vertexIndex)) {
                newThickness += thicknessChanges[vertexIndex];
                vertexModified = true;
            }
            
            if (ageChanges.count(vertexIndex)) {
                newAge = ageChanges[vertexIndex]; // Set new absolute age
                // Note: We don't count age changes as visual modifications anymore
            } else {
                // Apply general aging if no boundary interaction reset it
                newAge += ageIncreaseRate;
                // Don't count age changes as significant visual modifications anymore
                // Age changes don't affect the appearance of the planet in this simulation
            }

            // Clamp thickness
            newThickness = glm::clamp(newThickness, minThickness, maxThickness);
            
            // Only update thickness and set the modified flag if there was a significant thickness change
            if (std::abs(newThickness - currentThickness) > SIGNIFICANT_THICKNESS_CHANGE) {
                thicknessMap[vertexIndex] = newThickness;
                vertexModified = true;
            }
            
            // Always update age, but DO NOT count it for the dirty flag
            ageMap[vertexIndex] = newAge;
            
            if (vertexModified) {
                anyCrustModified = true;
            }
        }
    }
    
    return anyCrustModified;
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
