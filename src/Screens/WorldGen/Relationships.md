WorldGen/
├── Core/                           # Core structures and interfaces
│   ├── WorldGenParameters.h/.cpp   # Configuration parameters
│   ├── WorldGenSeed.h/.cpp         # Seed management
│   └── TerrainTypes.h              # Common terrain type definitions
│
├── Planet/                         # Planet-level systems
│   ├── PlanetData.h/.cpp           # Basic planet mesh data
│   ├── GlobeRenderer.h/.cpp        # Visualization of the planet
│   └── GravityModel.h/.cpp         # Gravity calculations
│
├── Lithosphere/                    # Earth's outer rigid layer
│   ├── Lithosphere.h/.cpp          # Main lithosphere simulation
│   ├── Crust/                      # The outermost layer of the lithosphere
│   │   ├── CrustGenerator.h/.cpp   # Generates different crust types
│   │   └── MountainGenerator.h/.cpp # Mountain formation at plate boundaries
│   └── Plate/                      # Tectonic plate systems
│       ├── TectonicPlate.h/.cpp    # Plate definition
│       ├── PlateGenerator.h/.cpp   # Creates initial plates
│       ├── PlateRenderer.h/.cpp    # Visualization of plates
│       └── BoundaryDetector.h/.cpp # Handles plate boundary calculations
│
├── Hydrosphere/                    # Water-related systems
│   ├── OceanGenerator.h/.cpp       # Ocean basin creation
│   ├── RiverGenerator.h/.cpp       # River formation
│   ├── LakeGenerator.h/.cpp        # Lake formation
│   └── WaterCycle.h/.cpp           # Water evaporation and precipitation
│
├── Atmosphere/                     # Atmospheric systems
│   ├── AtmosphereGenerator.h/.cpp  # Atmosphere creation
│   ├── WeatherSystem.h/.cpp        # Weather patterns
│   ├── ClimateModel.h/.cpp         # Climate zones
│   └── WindPatterns.h/.cpp         # Global wind patterns
│
├── Biosphere/                      # Biome and ecological systems
│   ├── BiomeGenerator.h/.cpp       # Biome determination
│   ├── BiomeTypes.h                # Biome classification
│   └── Vegetation.h/.cpp           # Vegetation distribution
│
├── Renderers/                      # Visualization and rendering
│   ├── TerrainRenderer.h/.cpp      # For rendering terrain
│   ├── BiomeRenderer.h/.cpp        # For rendering biomes
│   └── AtmosphereRenderer.h/.cpp   # For rendering atmospheric effects
│
└── UI/                             # User interface components
    ├── WorldGenUI.h/.cpp           # Main UI class
    ├── ParameterControls.h/.cpp    # UI for parameter adjustment
    └── WorldPreview.h/.cpp         # Preview visualization