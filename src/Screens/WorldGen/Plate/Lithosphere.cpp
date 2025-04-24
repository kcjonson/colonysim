#define GLM_ENABLE_EXPERIMENTAL
#include "Lithosphere.h"
// #include "PlateGenerator.h" // REMOVE: No longer needed here, included via Lithosphere.h indirectly if required
#include <glm/gtx/vector_angle.hpp> // For glm::angle
#include <glm/gtx/norm.hpp> // For glm::length
#include <algorithm> // For std::find_if, std::sort, std::unique, std::min/max
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

// Remove M_PI definition
// #ifndef M_PI // Define M_PI if not already defined (e.g., on Windows with MSVC)
// #define M_PI 3.14159265358979323846
// #endif


namespace WorldGen {

Lithosphere::Lithosphere(const PlanetParameters& parameters, uint64_t seed)
    : m_parameters(parameters)
    , m_random(seed)
{
    std::cout << "Lithosphere created with seed: " << seed << std::endl;
}

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

// --- Helper Method Implementations ---

void Lithosphere::GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates) {
    centers.clear();
    if (numPlates <= 0) return;
    std::cout << "Generating " << numPlates << " plate centers..." << std::endl; // Added log

    // Minimum distance between plate centers (adjusted based on number of plates)
    // Use angular distance for sphere
    // Reduce the factor from 1.8f to 1.2f for potentially better spacing
    float minAngleDistance = 1.2f * std::sqrt(4.0f * glm::pi<float>() / static_cast<float>(numPlates)); // Adjusted factor
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
        // Generate random point on sphere
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
    for (auto& plate : m_plates) {
        float plateMass = 0.0f;
        float initialThickness = (plate->GetType() == PlateType::Continental) ? 0.5f : 0.2f; // Example values
        float initialAge = (plate->GetType() == PlateType::Continental) ? 100.0f : 1.0f; // Example values

        const auto& vertexIndices = plate->GetVertexIndices();
        for (int vertexIndex : vertexIndices) {
            // TODO: Add noise or more complex initialization later
            plate->SetVertexCrustThickness(vertexIndex, initialThickness);
            plate->SetVertexCrustAge(vertexIndex, initialAge);

            // Assume vertex area is roughly constant for mass calculation for now
            // A better approach would use actual mesh face areas associated with the vertex
            plateMass += initialThickness; // Simplified mass contribution
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

void Lithosphere::DetectBoundaries(const std::vector<glm::vec3>& planetVertices, const std::vector<unsigned int>& planetIndices) {
    // std::cout << "Lithosphere::DetectBoundaries called." << std::endl; // Less verbose
    if (m_plates.empty() || planetIndices.empty()) {
        // std::cerr << "Cannot detect boundaries: No plates or indices." << std::endl; // Less verbose
        return;
    }

    // 1. Clear existing boundaries using the new method
    for (auto& plate : m_plates) {
        plate->ClearBoundaries();
    }

    // 2. Create a map from vertex index to plate ID for quick lookup
    std::unordered_map<int, int> vertexIndexToPlateId;
    for (const auto& plate : m_plates) {
        for (int vertexIndex : plate->GetVertexIndices()) {
            // Check for conflicts (a vertex should only belong to one plate)
            if (vertexIndexToPlateId.count(vertexIndex)) {
                 // This shouldn't happen with the current assignment logic, but good to check
                 std::cerr << "Warning: Vertex " << vertexIndex << " assigned to multiple plates (" << vertexIndexToPlateId[vertexIndex] << " and " << plate->GetId() << ")" << std::endl;
            }
            vertexIndexToPlateId[vertexIndex] = plate->GetId();
        }
    }

    // 3. Keep track of boundaries being built
    // Key: pair of plate IDs (smaller first), Value: The boundary struct being built
    std::map<std::pair<int, int>, PlateBoundary> boundaryDataMap;

    // 4. Iterate through triangles defined by indices to find edges
    for (size_t i = 0; i < planetIndices.size(); i += 3) {
        unsigned int v_indices[3] = { planetIndices[i], planetIndices[i + 1], planetIndices[i + 2] };

        // Check all 3 edges of the triangle
        for (int j = 0; j < 3; ++j) {
            unsigned int u_idx = v_indices[j];
            unsigned int v_idx = v_indices[(j + 1) % 3];

            // Ensure consistent edge representation (smaller index first)
            if (u_idx > v_idx) std::swap(u_idx, v_idx);

            auto it_u = vertexIndexToPlateId.find(u_idx);
            auto it_v = vertexIndexToPlateId.find(v_idx);

            if (it_u != vertexIndexToPlateId.end() && it_v != vertexIndexToPlateId.end()) {
                int plateIdU = it_u->second;
                int plateIdV = it_v->second;

                // If vertices are on different plates, this edge is a boundary edge
                if (plateIdU != plateIdV) {
                    int plate1Id = std::min(plateIdU, plateIdV);
                    int plate2Id = std::max(plateIdU, plateIdV);
                    std::pair<int, int> boundaryKey = {plate1Id, plate2Id};

                    // Get or create the boundary data in the map
                    PlateBoundary& boundary = boundaryDataMap[boundaryKey]; // Creates if not exists

                    // Initialize plate indices if it's a new boundary
                    if (boundary.plate1Index == 0 && boundary.plate2Index == 0) { // Check default init state (assuming IDs start from 1 or are non-zero)
                        boundary.plate1Index = plate1Id;
                        boundary.plate2Index = plate2Id;
                    }

                    // Add the shared vertices and the edge to the boundary
                    // Use sets to automatically handle duplicates
                    boundary.m_sharedVertexIndices.push_back(u_idx); // We'll unique this later if needed
                    boundary.m_sharedVertexIndices.push_back(v_idx);
                    boundary.m_sharedEdgeIndicesSet.insert({u_idx, v_idx});
                }
            }
        }
    }

    // 5. Finalize boundaries: unique vertices, convert edge set to vector, and add to plates
    for (auto& pair : boundaryDataMap) {
        PlateBoundary& boundary = pair.second;

        // Unique shared vertices (optional, depends on how they are used)
        std::sort(boundary.m_sharedVertexIndices.begin(), boundary.m_sharedVertexIndices.end());
        boundary.m_sharedVertexIndices.erase(std::unique(boundary.m_sharedVertexIndices.begin(), boundary.m_sharedVertexIndices.end()), boundary.m_sharedVertexIndices.end());

        // Convert edge set to vector
        boundary.m_sharedEdgeIndices.assign(boundary.m_sharedEdgeIndicesSet.begin(), boundary.m_sharedEdgeIndicesSet.end());

        // Add the finalized boundary to both involved plates
        TectonicPlate* plate1 = GetPlateById(boundary.plate1Index);
        TectonicPlate* plate2 = GetPlateById(boundary.plate2Index);
        if (plate1 && plate2) {
            plate1->AddBoundary(boundary);
            plate2->AddBoundary(boundary);
        } else {
            std::cerr << "Error: Could not find plates " << boundary.plate1Index << " or " << boundary.plate2Index << " during boundary finalization." << std::endl;
        }
    }

    // std::cout << "Detected " << boundaryDataMap.size() << " boundaries." << std::endl; // Less verbose
}

void Lithosphere::AnalyzeBoundaries(const std::vector<glm::vec3>& planetVertices) {
    // std::cout << "Lithosphere::AnalyzeBoundaries called." << std::endl; // Less verbose
    if (m_plates.empty()) return;

    // Use a map to store updated boundary data to avoid modifying while iterating
    // Key: pair of plate IDs (smaller first), Value: Updated PlateBoundary struct
    std::map<std::pair<int, int>, PlateBoundary> updatedBoundaries;

    // Iterate through each plate and its detected boundaries
    for (const auto& plate1_ptr : m_plates) {
        const TectonicPlate& plate1 = *plate1_ptr;
        for (const PlateBoundary& boundary : plate1.GetBoundaries()) {
            // Ensure we process each boundary pair only once
            int plate1Id = plate1.GetId();
            int plate2Id = (boundary.plate1Index == plate1Id) ? boundary.plate2Index : boundary.plate1Index;
            if (plate1Id >= plate2Id) continue; // Process only when plate1Id < plate2Id

            TectonicPlate* plate2_ptr = GetPlateById(plate2Id);
            if (!plate2_ptr) {
                std::cerr << "Error: Could not find plate " << plate2Id << " during boundary analysis." << std::endl;
                continue;
            }
            const TectonicPlate& plate2 = *plate2_ptr;

            // Get the current boundary data (might already be partially updated if shared)
            std::pair<int, int> boundaryKey = {plate1Id, plate2Id};
            PlateBoundary currentBoundary = boundary; // Start with existing data

            if (currentBoundary.m_sharedEdgeIndices.empty()) {
                // std::cout << "Skipping boundary analysis for plates " << plate1Id << "-" << plate2Id << " due to no shared edges." << std::endl;
                continue; // Skip if no shared edges (shouldn't happen often)
            }

            // Calculate average relative movement across the boundary
            glm::vec3 avgRelativeVelocity(0.0f);
            glm::vec3 avgBoundaryNormal(0.0f);
            glm::vec3 avgBoundaryPosition(0.0f);
            int edgeCount = 0;

            for (const auto& edge : currentBoundary.m_sharedEdgeIndices) {
                int v1_idx = edge.first;
                int v2_idx = edge.second;

                if (v1_idx >= planetVertices.size() || v2_idx >= planetVertices.size()) {
                    std::cerr << "Error: Vertex index out of bounds during boundary analysis." << std::endl;
                    continue;
                }

                glm::vec3 pos1 = planetVertices[v1_idx];
                glm::vec3 pos2 = planetVertices[v2_idx];
                glm::vec3 edgeMidpoint = glm::normalize((pos1 + pos2) * 0.5f);

                glm::vec3 velocity1 = plate1.CalculateMovementAt(edgeMidpoint);
                glm::vec3 velocity2 = plate2.CalculateMovementAt(edgeMidpoint);
                glm::vec3 relativeVelocity = velocity2 - velocity1;

                avgRelativeVelocity += relativeVelocity;

                // Calculate edge normal (tangent to sphere, perpendicular to edge)
                glm::vec3 edgeVector = glm::normalize(pos2 - pos1); // Direction along the edge
                glm::vec3 edgeNormal = glm::normalize(glm::cross(edgeMidpoint, edgeVector)); // Normal to edge, tangent to sphere
                avgBoundaryNormal += edgeNormal;
                avgBoundaryPosition += edgeMidpoint;
                edgeCount++;
            }

            if (edgeCount == 0) continue;

            avgRelativeVelocity /= static_cast<float>(edgeCount);
            avgBoundaryNormal = glm::normalize(avgBoundaryNormal / static_cast<float>(edgeCount));
            avgBoundaryPosition = glm::normalize(avgBoundaryPosition / static_cast<float>(edgeCount));

            // --- Classify Boundary Type --- //
            float relativeSpeed = glm::length(avgRelativeVelocity);
            currentBoundary.m_relativeMovementMagnitude = relativeSpeed;

            if (relativeSpeed < 1e-6f) { // Threshold for near-zero movement
                currentBoundary.type = BoundaryType::Transform; // Or maybe inactive?
                currentBoundary.m_convergenceSpeed = 0.0f;
                currentBoundary.m_transformSpeed = 0.0f;
                currentBoundary.stress = 0.0f;
            } else {
                // Project relative velocity onto the boundary normal
                // Convergence is positive dot product, Divergence is negative
                float convergenceComponent = glm::dot(avgRelativeVelocity, avgBoundaryNormal);

                // Project relative velocity onto the boundary tangent (perpendicular to normal)
                glm::vec3 tangentComponentVec = avgRelativeVelocity - convergenceComponent * avgBoundaryNormal;
                float transformComponent = glm::length(tangentComponentVec);

                currentBoundary.m_convergenceSpeed = convergenceComponent;
                currentBoundary.m_transformSpeed = transformComponent;

                // Simple classification based on dominant component
                float convergenceAbs = std::abs(convergenceComponent);
                float thresholdRatio = 0.707f; // ~cos(45 deg), adjust as needed

                if (convergenceComponent > relativeSpeed * thresholdRatio) {
                    currentBoundary.type = BoundaryType::Convergent;
                } else if (convergenceComponent < -relativeSpeed * thresholdRatio) {
                    currentBoundary.type = BoundaryType::Divergent;
                } else {
                    currentBoundary.type = BoundaryType::Transform;
                }

                // --- Calculate Stress (Simple Example) --- //
                // Stress could be proportional to convergence/transform speed, plate types, etc.
                float stressFactor = 1.0f; // Base factor
                if (currentBoundary.type == BoundaryType::Convergent) {
                    stressFactor = 2.0f * convergenceAbs; // Higher stress for faster convergence
                    // Consider plate types (Continental-Continental convergence is high stress)
                    if (plate1.GetType() == PlateType::Continental && plate2.GetType() == PlateType::Continental) {
                        stressFactor *= 1.5f;
                    }
                } else if (currentBoundary.type == BoundaryType::Divergent) {
                    stressFactor = 0.5f * std::abs(convergenceComponent); // Lower stress for divergence
                } else { // Transform
                    stressFactor = 1.0f * transformComponent; // Stress related to sliding speed
                }
                currentBoundary.stress = stressFactor * 10.0f; // Scale as needed
            }

            // Store the updated boundary data
            updatedBoundaries[boundaryKey] = currentBoundary;
        }
    }

    // Apply the updated boundary data back to the plates
    for (const auto& pair : updatedBoundaries) {
        const std::pair<int, int>& key = pair.first;
        const PlateBoundary& updatedBoundary = pair.second;
        TectonicPlate* plate1 = GetPlateById(key.first);
        TectonicPlate* plate2 = GetPlateById(key.second);

        if (plate1 && plate2) {
            plate1->UpdateBoundary(key.second, updatedBoundary);
            plate2->UpdateBoundary(key.first, updatedBoundary);
        } else {
             std::cerr << "Error: Could not find plates " << key.first << " or " << key.second << " when applying updated boundaries." << std::endl;
        }
    }
    // std::cout << "Finished analyzing boundaries." << std::endl; // Less verbose
}

// Placeholder implementations for private helper methods (to be filled in later steps)
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
