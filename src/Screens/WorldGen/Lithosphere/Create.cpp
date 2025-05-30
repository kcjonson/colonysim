/*
 * DEPRECATED - This class-based lithosphere implementation is no longer used.
 * The functional plate generation system in Generators/TectonicPlates.h/cpp is used instead.
 * This file is kept for reference but should not be used in new code.
 */
#define GLM_ENABLE_EXPERIMENTAL
#include "Lithosphere.h"
#include <glm/gtx/vector_angle.hpp> // For glm::angle
#include <glm/gtx/norm.hpp> // For glm::length
#include <algorithm> // For std::find_if, std::sort, std::unique, std::min/max
#include <iostream> // For debug output
#include <limits> // Required for std::numeric_limits
#include <cmath> // Required for std::sqrt, std::uniform_real_distribution, std::acos, std::abs
#include <random> // For std::uniform_real_distribution
#include <glm/gtc/constants.hpp> // For pi()


namespace WorldGen {

void Lithosphere::CreatePlates(const std::vector<glm::vec3>& planetVertices) {
    std::cout << "Lithosphere::CreatePlates called." << std::endl;
    m_plates.clear();
    std::vector<glm::vec3> centers;

    // 1. Generate plate centers
    GeneratePlateCenters(centers, m_parameters.numTectonicPlates);

    // 2. Create TectonicPlate objects
    for (int i = 0; i < centers.size(); ++i) {
        // Determine plate type - roughly 30% continental, 70% oceanic
        PlateType type = (m_random() % 100 < 30) ? PlateType::Continental : PlateType::Oceanic;
        m_plates.push_back(std::make_shared<TectonicPlate>(i, type, centers[i]));
    }
    std::cout << "Created " << m_plates.size() << " plate objects." << std::endl;

    // 3. Assign planet mesh vertices to plates
    AssignVerticesToPlates(planetVertices);

    // 4. Initialize plate properties (thickness, age, mass)
    InitializePlateProperties();

    // 5. Generate initial plate movements
    GeneratePlateMovements();

    std::cout << "Lithosphere::CreatePlates finished." << std::endl;
}


void Lithosphere::GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates) {
    centers.clear();
    if (numPlates <= 0) return;
    std::cout << "Generating " << numPlates << " plate centers..." << std::endl; // Added log

    // Minimum distance between plate centers (adjusted based on number of plates)
    // Use angular distance for sphere
    // Reduce the factor further for potentially better spacing
    float minAngleDistance = 0.8f * std::sqrt(4.0f * glm::pi<float>() / static_cast<float>(numPlates)); // Adjusted factor
    std::cout << "Minimum angle distance: " << minAngleDistance << std::endl; // Added log

    // Generate first point randomly
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    glm::vec3 firstPoint;
    do {
        firstPoint = glm::vec3(
            dist(m_random),
            dist(m_random),
            dist(m_random)
        );
    } while (glm::length(firstPoint) < 0.001f); // Avoid zero vector

    // Normalize to put on unit sphere
    firstPoint = glm::normalize(firstPoint);
    centers.push_back(firstPoint);

    // Try to generate remaining points using Poisson disk sampling on a sphere
    int attempts = 0;
    const int maxAttemptsPerPlate = 100; // Limit attempts per plate
    const int maxTotalAttempts = numPlates * maxAttemptsPerPlate;
    std::cout << "Max total attempts: " << maxTotalAttempts << std::endl; // Added log

    while (centers.size() < numPlates && attempts < maxTotalAttempts) {
        if (attempts > 0 && attempts % 1000 == 0) { // Log every 1000 attempts
            std::cout << "GeneratePlateCenters attempt " << attempts << ", centers found: " << centers.size() << std::endl;
        }

        /* 
        Generate random point on sphere
        This loop repeatedly generates random 3D points within a cube from (-1,-1,-1) to (1,1,1) until it finds one that is not extremely close to the origin (0,0,0). 
        This avoids potential issues with zero vectors, especially since the next step in the surrounding code is to normalize 
        this candidate vector (dividing by its length), which would fail if the length were zero.
        */
        glm::vec3 candidate;
        do {
            candidate = glm::vec3(
                dist(m_random),
                dist(m_random),
                dist(m_random)
            );
        } while (glm::length(candidate) < 0.001f);

        candidate = glm::normalize(candidate);

        // Check distance to existing points
        bool tooClose = false;
        for (const auto& center : centers) {
            // Use angle between vectors as distance measure on sphere
            float angle = glm::angle(center, candidate);
            if (angle < minAngleDistance) {
                tooClose = true;
                break;
            }
        }

        // If not too close to any existing point, add it
        if (!tooClose) {
            centers.push_back(candidate);
        }

        attempts++;
    }
    std::cout << "GeneratePlateCenters finished after " << attempts << " attempts. Centers generated: " << centers.size() << std::endl; // Added log
    if (centers.size() < numPlates) {
         std::cerr << "Warning: Could only generate " << centers.size() << " / " << numPlates << " plate centers with current settings." << std::endl;
    }
     std::cout << "Generated " << centers.size() << " plate centers." << std::endl;
}

void Lithosphere::AssignVerticesToPlates(const std::vector<glm::vec3>& planetVertices) {
    if (m_plates.empty()) return;

    // Clear previous assignments using the new method
    for (auto& plate : m_plates) {
        plate->ClearVertices();
    }

    int assignedCount = 0;
    for (int i = 0; i < planetVertices.size(); ++i) {
        const glm::vec3& vertexPos = planetVertices[i]; // Assume already normalized

        // Find closest plate center using spherical distance (angle)
        int closestPlateIndex = -1;
        float minAngle = std::numeric_limits<float>::max();

        for (size_t k = 0; k < m_plates.size(); ++k) {
            // Use dot product for angle calculation (more efficient)
            // cos(angle) = dot(v1, v2) / (len(v1)*len(v2))
            // Since vectors are normalized, cos(angle) = dot(v1, v2)
            // We want the minimum angle, which corresponds to the maximum dot product.
            float dotProd = glm::dot(vertexPos, m_plates[k]->GetCenter());
            // Clamp dot product to avoid potential floating point issues with acos
            dotProd = glm::clamp(dotProd, -1.0f, 1.0f);
            float angle = glm::acos(dotProd); // Calculate actual angle

            if (angle < minAngle) {
                minAngle = angle;
                closestPlateIndex = static_cast<int>(k);
            }
        }

        // Assign vertex to the closest plate
        if (closestPlateIndex != -1) {
            m_plates[closestPlateIndex]->AddVertex(i);
            assignedCount++;
        }
    }
    // std::cout << "Assigned " << assignedCount << " vertices to plates." << std::endl; // Less verbose

    // Verify assignment (optional debug check)
    // for(const auto& plate : m_plates) {
    //     if (plate->GetVertexIndices().empty()) {
    //         std::cerr << "Warning: Plate " << plate->GetId() << " has no vertices assigned!" << std::endl;
    //     }
    // }
}

void Lithosphere::InitializePlateProperties() {
    float totalMassRecalc = 0.0f; // For debug
    std::uniform_real_distribution<float> noiseDist(-0.05f, 0.05f); // Add up to ±0.05 noise
    for (auto& plate : m_plates) {
        float plateMass = 0.0f;
        float initialThickness = (plate->GetType() == PlateType::Continental) ? 0.5f : 0.2f; // Example values
        float initialAge = (plate->GetType() == PlateType::Continental) ? 100.0f : 1.0f; // Example values

        const auto& vertexIndices = plate->GetVertexIndices();
        for (int vertexIndex : vertexIndices) {
            // Add random noise to thickness
            float noisyThickness = initialThickness + noiseDist(m_random);
            noisyThickness = glm::clamp(noisyThickness, 0.01f, 2.0f); // Clamp to valid range
            plate->SetVertexCrustThickness(vertexIndex, noisyThickness);
            plate->SetVertexCrustAge(vertexIndex, initialAge);
            plateMass += noisyThickness; // Use noisy thickness for mass
        }
        plate->SetTotalMass(plateMass);
        totalMassRecalc += plateMass;
    }
    std::cout << "Initialized plate properties. Approx total mass: " << totalMassRecalc << std::endl;
}

void Lithosphere::GeneratePlateMovements() {
    for (auto& plate : m_plates) {
        // Generate random movement direction tangent to sphere at plate center
        glm::vec3 normal = glm::normalize(plate->GetCenter());

        // Generate random vector
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        glm::vec3 randomVec(
            dist(m_random),
            dist(m_random),
            dist(m_random)
        );

        // Project onto tangent plane
        glm::vec3 movement = randomVec - normal * glm::dot(randomVec, normal);

        // Normalize and scale to a reasonable speed (adjust as needed)
        float speed = 0.005f; // Example speed, units depend on simulation time step
        if (glm::length(movement) > 0.001f) {
            movement = glm::normalize(movement) * speed;
        }

        plate->SetMovementVector(movement);

        // Set random rotation rate (adjust range as needed)
        float maxRotationRate = 0.002f; // Example max rate
        float rotationRate = dist(m_random) * maxRotationRate;
        plate->SetRotationRate(rotationRate);
    }
    std::cout << "Generated initial plate movements." << std::endl;
}


} // namespace WorldGen
