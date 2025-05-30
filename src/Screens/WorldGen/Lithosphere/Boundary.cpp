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
#include <map> // Use map for ordered boundary processing
#include <unordered_map> // For vertex->plate lookup
#include <set> // For ordered set to avoid duplicate edges/vertices and processed boundaries
#include <cmath> // For std::abs


namespace WorldGen {

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


} // namespace WorldGen
