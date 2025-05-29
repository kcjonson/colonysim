#pragma once

#include <memory>
#include "../Core/WorldGenParameters.h"
#include "World.h"

namespace WorldGen {
namespace Generators {

/**
 * @brief Factory class for creating World objects.
 * 
 * This class creates World objects based on parameters defined in WorldGenParameters.
 * It encapsulates the logic for determining the appropriate subdivision level
 * and distortion factor based on the desired resolution.
 */
class Generator {
public:    /**
     * @brief Create a new world using the specified parameters.
     * 
     * @param params The parameters to use for world generation.
     * @param progressTracker Optional progress tracker to report generation progress.
     * @return std::unique_ptr<World> A unique pointer to the newly created World.
     */
    static std::unique_ptr<World> CreateWorld(const PlanetParameters& params, uint64_t seed, std::shared_ptr<ProgressTracker> progressTracker = nullptr);

    /**
     * @brief Get the appropriate subdivision level for a given resolution.
     * 
     * @param resolution The desired resolution.
     * @return int The appropriate subdivision level.
     */
    static int GetSubdivisionLevel(int resolution);

    /**
     * @brief Calculate the number of tiles that will be generated for a given subdivision level.
     * 
     * @param subdivisionLevel The subdivision level.
     * @return int The approximate number of tiles.
     */
    static int CalculateTileCount(int subdivisionLevel);
};

} // namespace Generators
} // namespace WorldGen