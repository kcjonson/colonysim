# Plate Tectonics Integration Plan

This document outlines the plan to integrate the simulation logic from the `plate-tectonics` library into the `ColonySim` world generation system, adapting it for vector data on a spherical planet mesh.

## 1. Summary of `plate-tectonics` Approach

The `plate-tectonics` library simulates plate movement and interaction on a 2D grid. Key components include:

*   **`lithosphere`:** Manages the overall simulation, including a collection of `plate` objects, global height/age/index maps, and the main simulation loop (`update`).
*   **`plate`:** Represents a single tectonic plate, managing its own crust thickness (`map`), crust age (`age_map`), boundaries (`_bounds`), movement physics (`_movement`), and internal segmentation (`_segments`).
*   **Simulation Loop:** In each step, plates move, collisions are detected, and crust is modified based on interactions (subduction, orogeny, rifting). It handles crust aggregation (continents merging) and applies friction.
*   **Data:** Primarily uses 2D arrays (heightmaps, index maps) to represent the world state.

## 2. Proposed Data Format and Class Enhancements

Adapt the concepts from `plate-tectonics` to the existing `TectonicPlate` class and the vector/sphere context.

*   **`TectonicPlate.h` Enhancements:**
    *   **Per-Vertex Data:** Store properties associated with the vertices of the planet mesh assigned to this plate.
        *   `std::unordered_map<int, float> m_vertexCrustThickness;` // Maps vertex index to crust thickness
        *   `std::unordered_map<int, float> m_vertexCrustAge;`      // Maps vertex index to crust age
        *   *(Potentially others like density, rock type later)*
    *   **Movement:** Keep `m_movementVector` and `m_rotationRate`. Consider a more sophisticated `Movement` class later for spherical rotations.
    *   **Mass:** Add `float m_totalMass;` calculated from `m_vertexCrustThickness`.
    *   **Plate Center:** `m_center` remains important.
    *   **Vertex Indices:** Replace `m_tileIndices` with `std::vector<int> m_vertexIndices;`.
*   **`PlateBoundary` Struct Enhancements (`TectonicPlate.h`):**
    *   **Representation:** Store shared features of the underlying planet mesh.
        *   `std::vector<int> m_sharedVertexIndices;` // Indices of vertices lying on the boundary
        *   `std::vector<std::pair<int, int>> m_sharedEdgeIndices;` // Indices of edges forming the boundary (pair of vertex indices)
    *   **Properties:** Keep `stress` and `type`. Add `float m_relativeMovementMagnitude;` `m_convergenceSpeed`, `m_transformSpeed`.

## 3. Proposed File Structure

*   **`src/Screens/WorldGen/Plate/TectonicPlate.h/.cpp`:** Modify as described above.
*   **`src/Screens/WorldGen/Plate/PlateGenerator.h/.cpp`:** Refactor. Becomes the orchestrator, calling `Lithosphere`.
*   **`src/Screens/WorldGen/Plate/Lithosphere.h/.cpp`:** **(New Files)** Manages `TectonicPlate` objects and the simulation loop (`Update`), adapting logic from `plate-tectonics/lithosphere`.
*   **`src/Screens/WorldGen/Plate/Movement.h/.cpp`:** **(New Files - Optional/Later)** Dedicated class for spherical plate movement physics, adapting `plate-tectonics/movement`.
*   **`src/Screens/WorldGen/Plate/Noise.h/.cpp`:** **(New Files - Optional)** Helper functions for noise on a sphere, adapting `plate-tectonics/noise`.
*   **`src/Screens/WorldGen/Plate/PlateRenderer.h/.cpp`:** Update to use new `PlateBoundary` representation and visualize per-vertex data.

## 4. Proposed Implementation Steps (Incremental)

This plan aims to keep the application runnable after each major step.

*   **Step 1: Refactor `TectonicPlate` Data Structures** ✅
    *   Modify `TectonicPlate.h/.cpp` with new members (`m_vertexCrustThickness`, `m_vertexCrustAge`, `m_totalMass`, `m_vertexIndices`) and update `PlateBoundary`.
    *   Update `PlateGenerator.cpp` and `PlateRenderer.cpp` minimally to compile.
    *   *Goal:* Code compiles, existing generation might be visually incorrect or crash.
*   **Step 2: Introduce `Lithosphere` Class** ✅
    *   Create basic `Lithosphere.h/.cpp` with `std::vector<std::shared_ptr<TectonicPlate>> m_plates;`.
    *   Add constructor and placeholder methods (`CreatePlates`, `Update`).
    *   Modify `PlateGenerator` to create and hold a `Lithosphere` instance. `PlateGenerator::GetLithosphere()` provides access. `WorldGen.cpp` button action calls `lithosphere->CreatePlates()` and `lithosphere->DetectBoundaries()`. `WorldGen::Update` calls `lithosphere->Update()`.
    *   *Goal:* Code compiles, basic structure in place, generation does nothing yet.
*   **Step 3: Implement Initial Plate Generation in `Lithosphere`** ✅
    *   Implement `Lithosphere::CreatePlates`.
    *   Adapt `PlateGenerator::GeneratePlateCenters` logic into `Lithosphere`.
    *   Implement vertex assignment based on closest plate center (spherical distance) in `Lithosphere::AssignVerticesToPlates`.
    *   Initialize basic plate properties (`PlateType`, `m_vertexCrustThickness`, `m_vertexCrustAge`, `m_totalMass`) in `Lithosphere::InitializePlateProperties`.
    *   Adapt `PlateGenerator::GeneratePlateMovements` logic into `Lithosphere::GeneratePlateMovements`.
    *   *Goal:* Plates are generated with initial properties and vertex assignments. No movement or interaction yet.
*   **Step 4: Implement Boundary Detection in `Lithosphere`** ✅
    *   Implement `Lithosphere::DetectBoundaries`.
    *   Iterate through planet mesh edges (derived from `planetIndices`). If vertices belong to different plates (using `vertexIndexToPlateId` map), store the edge/vertices in `PlateBoundary` for both plates using a temporary map.
    *   Update `PlateRenderer` to draw boundaries using `m_sharedEdgeIndices`.
    *   *Goal:* Plate boundaries are correctly identified and rendered.
*   **Step 5: Implement Basic Plate Movement in `Lithosphere`** ✅
    *   Implement movement in `Lithosphere::MovePlates(float deltaTime)`.
    *   Update plate `m_center` using spherical geometry (rotation based on `m_movementVector`).
    *   Update `m_movementVector` based on plate rotation (`m_rotationRate`).
    *   **Crucially:** Call `AssignVerticesToPlates` and `DetectBoundaries` within `Lithosphere::Update` after `MovePlates`.
    *   *Goal:* Plates appear to move, boundaries shift. No crust modification yet.
*   **Step 6: Implement Boundary Analysis (Type & Stress)** ✅
    *   Implement `Lithosphere::AnalyzeBoundaries`.
    *   Calculate average relative movement (`avgRelativeVelocity`) across boundary edges.
    *   Calculate average boundary normal (`avgBoundaryNormal`).
    *   Determine `BoundaryType` (Convergent, Divergent, Transform) based on projection of `avgRelativeVelocity` onto `avgBoundaryNormal`. Store `m_convergenceSpeed` and `m_transformSpeed`.
    *   Calculate `stress` based on speeds and plate types. Store in `PlateBoundary`.
    *   Update `PlateRenderer` to color boundaries based on `type` or `stress` (TODO later).
    *   Call `AnalyzeBoundaries` within `Lithosphere::Update`.
    *   *Goal:* Boundaries are classified and stress is calculated.
*   **Step 7: Implement Crust Modification (Convergent/Divergent)** ✅
    *   Implement `Lithosphere::ModifyCrust`.
    *   Iterate through boundaries. For shared vertices, modify `m_vertexCrustThickness` based on `BoundaryType` and speeds (subduction, orogeny, rifting).
    *   Reset `m_vertexCrustAge` at divergent boundaries. Increment age elsewhere.
    *   Implement `Lithosphere::RecalculatePlateMasses` based on updated `m_vertexCrustThickness`.
    *   Call `ModifyCrust` and `RecalculatePlateMasses` within `Lithosphere::Update`.
    *   Modify `PlateGenerator::GenerateElevationData` (or move to `Lithosphere`) to use `m_vertexCrustThickness` (TODO later).
    *   *Goal:* Basic geological features start forming based on interactions.
*   **Step 8: Refinement and Integration**
    *   Refine crust modification logic (e.g., volcanism, erosion).
    *   Adapt friction/aggregation from `plate-tectonics` if desired.
    *   Ensure `PlateRenderer` visualizes elevation/thickness/age (TODO later).
    *   Clean up code, add comments, ensure stability.

## 5. Addressing Vector/Sphere Challenges

*   **Raster to Vector:** Use mesh vertex/edge indices instead of grid indices. Use mesh adjacency (derived from triangle indices) instead of grid neighbor checks.
*   **Flat to Sphere:** Use spherical distance (angle between normalized vectors). Use vector math suitable for spheres (rotations around arbitrary axes, cross products for tangents/normals). Map noise correctly (TODO later).
