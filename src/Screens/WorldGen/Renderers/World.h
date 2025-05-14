#pragma once

#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "../Generators/World.h"

namespace WorldGen {
namespace Renderers {

/**
 * @brief Renders a World object on the screen.
 * 
 * This class provides visualization for the World object,
 * including options to visualize the mesh in different ways.
 */
class World {
public:
    /**
     * @brief Enum defining different visualization modes.
     */
    enum class RenderMode {
        Wireframe,      ///< Render tile edges only
        Solid,          ///< Render tiles with solid colors
        TileType,       ///< Color by tile type (pentagon or hexagon)
        Debug          ///< Debug visualization
    };

    /**
     * @brief Construct a new World renderer.
     */
    World();
    
    /**
     * @brief Destroy the World renderer.
     */
    ~World();

    /**
     * @brief Set the world to render.
     * 
     * @param world Pointer to the world to render.
     */
    void SetWorld(const Generators::World* world);

    /**
     * @brief Set the rendering mode.
     * 
     * @param mode The rendering mode to use.
     */
    void SetRenderMode(RenderMode mode);    /**
     * @brief Render the world.
     * 
     * @param viewMatrix The view matrix.
     * @param projectionMatrix The projection matrix.
     */
    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Validate the geometry of all tiles before rendering.
     * 
     * Checks for issues like missing vertices, non-planar tiles, etc.
     */
    void ValidateTileGeometry();

private:
    /**
     * @brief Generate rendering data for the world.
     */
    void GenerateRenderingData();

    /**
     * @brief Render the world as a wireframe.
     */
    void RenderWireframe(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Render the world as solid tiles.
     */
    void RenderSolid(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Render the world colored by tile type.
     */
    void RenderByTileType(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);

    /**
     * @brief Render debug visualization.
     */
    void RenderDebug(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);    /**
     * @brief Structure to hold triangle fan information for each tile
     */
    struct TileFanInfo {
        unsigned int startIndex;    ///< Starting index in the vertex buffer
        unsigned int vertexCount;   ///< Number of vertices in this tile (center + perimeter)
        unsigned int indexCount;    ///< Number of indices for this tile's triangle fan
    };
    
    const Generators::World* m_world;    ///< The world to render
    RenderMode m_renderMode;             ///< Current rendering mode
      // OpenGL rendering data
    unsigned int m_vao;                  ///< Vertex array object
    unsigned int m_vbo;                  ///< Vertex buffer object
    unsigned int m_ebo;                  ///< Element buffer object
    unsigned int m_shaderProgram;        ///< Shader program
    std::vector<TileFanInfo> m_tileFanInfo; ///< Triangle fan information for each tile
    
    /**
     * @brief Compile and link the shaders.
     * 
     * @return The shader program ID.
     */
    unsigned int compileShaders();

    // Render data for different modes
    std::vector<float> m_vertexData;     ///< Vertex data for rendering
    std::vector<unsigned int> m_indices; ///< Indices for rendering
      bool m_dataGenerated;                ///< Whether rendering data has been generated
};

} // namespace Renderers
} // namespace WorldGen