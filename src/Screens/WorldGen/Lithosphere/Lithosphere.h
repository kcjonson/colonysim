#pragma once

#include <vector>
#include <memory>
#include <random>
#include <glm/glm.hpp>
#include <map> // Added for boundary map
#include "Plate/TectonicPlate.h" // Updated path to TectonicPlate.h
#include "../Core/WorldGenParameters.h" // Updated path to WorldGenParameters.h

namespace WorldGen {

class Lithosphere {
public:
    // Use PlanetParameters from WorldGenParameters.h
    Lithosphere(const PlanetParameters& parameters, uint64_t seed);

    // Creates the initial set of tectonic plates.
    // Needs planet mesh vertices for assignment.
    void CreatePlates(const std::vector<glm::vec3>& planetVertices);

    // Runs one step of the simulation.
    // Needs planet mesh data for boundary detection and analysis.
    // Returns true if plates were modified during this update
    bool Update(float deltaTime, const std::vector<glm::vec3>& planetVertices, const std::vector<unsigned int>& planetIndices);

    // Getter for the plates
    const std::vector<std::shared_ptr<TectonicPlate>>& GetPlates() const;
    std::vector<std::shared_ptr<TectonicPlate>>& GetPlates();

    // Public helpers needed for initial generation
    void DetectBoundaries(const std::vector<glm::vec3>& planetVertices, const std::vector<unsigned int>& planetIndices);
    void AnalyzeBoundaries(const std::vector<glm::vec3>& planetVertices); // Pass vertices


private:
    // Store parameters by value, not reference
    PlanetParameters m_parameters;
    std::mt19937_64 m_random;
    std::vector<std::shared_ptr<TectonicPlate>> m_plates;

    // Helper methods
    void GeneratePlateCenters(std::vector<glm::vec3>& centers, int numPlates);
    void AssignVerticesToPlates(const std::vector<glm::vec3>& planetVertices);
    void InitializePlateProperties();
    void GeneratePlateMovements();
    bool MovePlates(float deltaTime); // Return true if any plates moved
    bool ModifyCrust(float deltaTime); // Return true if crust was modified
    void RecalculatePlateMasses();

    // Helper to get plate by ID
    TectonicPlate* GetPlateById(int id);

};

} // namespace WorldGen
