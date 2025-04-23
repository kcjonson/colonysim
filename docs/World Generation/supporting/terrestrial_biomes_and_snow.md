# Terrestrial Biomes and Snow/Glacier System

## Overview

This document details the enhanced terrestrial biome classification system and snow/glacier calculation component for the realistic world generation system. These enhancements will provide more detailed and scientifically accurate biome representation in the initial planet view, along with realistic snow and glacier formation based on climate conditions.

## 1. Terrestrial Biome Classification

### 1.1 Classification Framework

The biome classification system uses a modified Whittaker approach that classifies biomes primarily based on temperature and precipitation, with additional influences from elevation, latitude, and other factors. This approach provides a scientifically grounded method for determining realistic biome distribution across the planet.

### 1.2 Primary Classification Parameters

- **Annual Mean Temperature**: Average temperature throughout the year (°C)
- **Temperature Seasonality**: Variation in temperature between seasons
- **Annual Precipitation**: Total yearly precipitation (mm)
- **Precipitation Seasonality**: Distribution of precipitation throughout the year
- **Elevation**: Height above sea level (m)
- **Latitude**: Distance from equator (°)

### 1.3 Detailed Terrestrial Biome Types

#### 1.3.1 Forest Biomes

| Biome Type | Temperature Range (°C) | Precipitation Range (mm/year) | Characteristics |
|------------|------------------------|-------------------------------|-----------------|
| Tropical Rainforest | >20 | >2000 | High biodiversity, year-round growing season, multi-layered canopy |
| Tropical Seasonal Forest | >20 | 1000-2000 | Distinct wet and dry seasons, deciduous or semi-deciduous |
| Temperate Deciduous Forest | 5-20 | 750-1500 | Distinct seasons, broad-leaved trees that shed leaves seasonally |
| Temperate Rainforest | 5-15 | >1400 | Mild temperatures, high humidity, dominated by coniferous trees |
| Boreal Forest (Taiga) | -5-5 | 300-850 | Long winters, short summers, dominated by coniferous trees |
| Montane Forest | Varies with elevation | Varies with elevation | Vegetation zones change with altitude |

#### 1.3.2 Grassland Biomes

| Biome Type | Temperature Range (°C) | Precipitation Range (mm/year) | Characteristics |
|------------|------------------------|-------------------------------|-----------------|
| Tropical Savanna | >20 | 500-1300 | Seasonal precipitation, mixture of grasses and scattered trees |
| Temperate Grassland | 0-20 | 250-750 | Hot summers, cold winters, dominated by grasses with few trees |
| Alpine Grassland | <10 | Varies | Cool temperatures, short growing season, above tree line |

#### 1.3.3 Desert and Xeric Biomes

| Biome Type | Temperature Range (°C) | Precipitation Range (mm/year) | Characteristics |
|------------|------------------------|-------------------------------|-----------------|
| Hot Desert | >18 | <250 | High temperatures, extreme daily variation, sparse vegetation |
| Cold Desert | <18 | <250 | Low temperatures, often in rain shadows, adapted vegetation |
| Semi-Desert | Varies | 250-500 | More vegetation than true deserts, transitional zones |
| Xeric Shrubland | 10-20 | 250-600 | Dominated by drought-resistant shrubs, Mediterranean climates |

#### 1.3.4 Tundra and Cold Biomes

| Biome Type | Temperature Range (°C) | Precipitation Range (mm/year) | Characteristics |
|------------|------------------------|-------------------------------|-----------------|
| Arctic Tundra | <0 (annual average) | 150-250 | Permafrost layer, no trees, mosses and lichens |
| Alpine Tundra | Varies with elevation | Varies | Similar to arctic tundra but at high elevations |
| Polar Desert | <0 (annual average) | <100 | Extremely cold, minimal vegetation |

#### 1.3.5 Wetland Biomes (Terrestrial)

| Biome Type | Temperature Range (°C) | Precipitation Range (mm/year) | Characteristics |
|------------|------------------------|-------------------------------|-----------------|
| Temperate Wetland | 5-20 | >750 | Saturated soils, water-tolerant vegetation |
| Tropical Wetland | >20 | >1500 | Saturated soils, high biodiversity |

### 1.4 Biome Transition Zones

The system implements natural transitions between biomes rather than abrupt boundaries. These ecotones are calculated using:

- Weighted blending between adjacent biome types
- Noise functions to create natural variation
- Topographical influences on transition boundaries

## 2. Snow and Glacier System

### 2.1 Snow Formation Conditions

Snow formation is calculated based on:

- **Temperature**: Snow forms when temperatures are below freezing (0°C)
- **Precipitation**: Sufficient moisture must be available for snowfall
- **Seasonality**: Determined by latitude and elevation
- **Persistence**: Based on temperature patterns and solar radiation

The system distinguishes between:

1. **Seasonal Snow Cover**: Temporary snow that melts during warm seasons
2. **Permanent Snow Fields**: Snow that persists year-round but hasn't formed glacial ice

### 2.2 Glacier Formation Conditions

Glaciers form under specific conditions:

- **Snow Accumulation**: Areas where annual snowfall exceeds annual melt
- **Temperature**: Cold enough to preserve snow year-round
- **Compression**: Sufficient snow depth to compress lower layers into ice
- **Topography**: Suitable terrain for snow accumulation and ice flow
- **Time**: Sufficient time for snow to compress into glacial ice

The system models:

1. **Mountain Glaciers**: Formed in high mountain valleys
2. **Ice Caps**: Dome-shaped ice masses covering mountain tops and high plateaus

### 2.3 Calculation Process

The snow and glacier calculation process follows these steps:

1. **Monthly Climate Simulation**:
   - Calculate temperature and precipitation for each month
   - Determine snow accumulation and melt rates

2. **Snow Persistence Determination**:
   - Track snow depth throughout the year
   - Identify areas where snow persists year-round

3. **Glacier Formation Analysis**:
   - Evaluate permanent snow areas for glacier formation potential
   - Consider elevation, slope, and neighboring conditions

4. **Glacier Flow Calculation**:
   - Determine thickness based on accumulation rate and time
   - Calculate flow direction and speed based on topography

## 3. Implementation Details

### 3.1 Data Structures

```cpp
// Snow and glacier data for each tile
struct SnowData {
    bool hasSeasonalSnow;       // Whether the tile has seasonal snow
    float maxSnowDepth;         // Maximum snow depth in meters
    int snowMonthsPerYear;      // Number of months with snow cover
    bool isPermanentSnow;       // Whether snow persists year-round
    bool isGlacier;             // Whether the tile contains glacial ice
    float glacierThickness;     // Thickness of glacier in meters
    float glacierFlowDirection; // Direction of glacier flow in radians
    float glacierFlowSpeed;     // Speed of glacier flow in meters/year
};

// Biome data for each tile
struct BiomeData {
    BiomeType primaryBiome;     // Primary biome classification
    BiomeType secondaryBiome;   // Secondary biome for transition zones
    float transitionFactor;     // Blend factor between primary and secondary
    float vegetationDensity;    // Density of vegetation (0.0-1.0)
};
```

### 3.2 Key Algorithms

#### 3.2.1 Biome Classification

```cpp
BiomeData ClassifyBiome(float temperature, float precipitation, float elevation, float latitude) {
    BiomeData result;
    
    // Find primary biome based on Whittaker classification
    result.primaryBiome = DeterminePrimaryBiome(temperature, precipitation);
    
    // Apply elevation and latitude adjustments
    result.primaryBiome = AdjustForElevationAndLatitude(result.primaryBiome, elevation, latitude);
    
    // Determine transition zones
    result = CalculateTransitions(result, temperature, precipitation, elevation, latitude);
    
    // Calculate vegetation density
    result.vegetationDensity = CalculateVegetationDensity(result.primaryBiome, temperature, precipitation);
    
    return result;
}
```

#### 3.2.2 Snow Calculation

```cpp
SnowData CalculateSnowConditions(float annualTemperature, float temperatureVariation, 
                                float annualPrecipitation, float precipitationSeasonality,
                                float elevation, float latitude) {
    SnowData result;
    
    // Calculate monthly conditions
    for (int month = 0; month < 12; month++) {
        float monthlyTemperature = CalculateMonthlyTemperature(
            annualTemperature, temperatureVariation, latitude, month);
        
        float monthlyPrecipitation = CalculateMonthlyPrecipitation(
            annualPrecipitation, precipitationSeasonality, latitude, month);
        
        // Calculate snow accumulation and melt
        float snowAccumulation = CalculateSnowAccumulation(monthlyTemperature, monthlyPrecipitation);
        float snowMelt = CalculateSnowMelt(monthlyTemperature, currentSnowDepth, latitude);
        
        // Update snow depth
        currentSnowDepth += snowAccumulation - snowMelt;
        currentSnowDepth = max(0.0f, currentSnowDepth);
        
        // Track statistics
        if (currentSnowDepth > 0) snowMonths++;
        maxSnowDepth = max(maxSnowDepth, currentSnowDepth);
    }
    
    // Set snow properties
    result.maxSnowDepth = maxSnowDepth;
    result.snowMonthsPerYear = snowMonths;
    result.hasSeasonalSnow = (snowMonths > 0);
    result.isPermanentSnow = (snowMonths == 12);
    
    return result;
}
```

#### 3.2.3 Glacier Formation

```cpp
bool CanFormGlacier(const SnowData& snowData, float elevation, float slope, float planetAge) {
    // Basic requirements
    if (!snowData.isPermanentSnow) return false;
    
    // Check snow accumulation rate
    float annualAccumulation = snowData.maxSnowDepth / SNOW_TO_WATER_RATIO;
    if (annualAccumulation < MIN_ANNUAL_SNOW_FOR_GLACIER) return false;
    
    // Check planet age (needs time to form)
    if (planetAge * 1e9 < MIN_YEARS_FOR_GLACIER_FORMATION) return false;
    
    // Check slope (too steep = avalanche, too flat = no flow)
    if (slope > 45.0f) return false;
    
    return true;
}
```

### 3.3 Visualization

The biome and snow/glacier systems will be visualized in the planet view with:

- **Biome Coloration**: Distinct color schemes for each biome type
- **Snow Representation**: White overlay for snow-covered areas
- **Glacier Visualization**: Textured ice appearance for glaciers
- **Seasonal Variation**: Option to view different seasons

## 4. Integration with World Generation

### 4.1 Generation Pipeline

The biome and snow/glacier systems integrate into the world generation pipeline as follows:

1. **Tectonic Plate Simulation**: Creates continental landmasses and mountain ranges
2. **Elevation Generation**: Determines terrain height and topography
3. **Atmospheric Circulation**: Calculates global wind patterns
4. **Temperature Calculation**: Determines temperature patterns based on latitude, elevation, etc.
5. **Precipitation Calculation**: Determines rainfall patterns based on atmospheric circulation
6. **River Formation**: Simulates water flow from high to low elevations
7. **Lake Formation**: Creates lakes in depressions where water accumulates
8. **Biome Classification**: Assigns biomes based on climate conditions
9. **Snow and Glacier Calculation**: Determines snow cover and glacier formation

### 4.2 User Interface

The planet view UI will include:

- **Biome Legend**: Color-coded explanation of visible biomes
- **Season Selection**: Ability to view different seasonal conditions
- **Detail Level**: Controls for adjusting visualization detail
- **Information Panel**: Details about selected regions

## 5. Performance Considerations

### 5.1 Optimization Strategies

- **Multi-threading**: Parallel processing of independent tile calculations
- **Spatial Partitioning**: Regional calculations for large worlds
- **Caching**: Store intermediate results for temperature and precipitation
- **Level-of-Detail**: Adjust calculation precision based on view distance

### 5.2 Memory Management

- **Data Compression**: Efficient storage of biome and snow data
- **Streaming**: Load detailed data only for visible regions
- **Procedural Generation**: Generate fine details on-demand

## 6. Future Extensions

### 6.1 Dynamic Weather System

The initial snow calculation provides the foundation for a dynamic weather system during gameplay:

- **Seasonal Changes**: Snow accumulation and melt based on season
- **Weather Events**: Snowstorms, blizzards, and thaws
- **Climate Shifts**: Long-term changes in snow and glacier patterns

### 6.2 Ecological Succession

Future versions could model changes in biomes over time:

- **Primary Succession**: Development of ecosystems on newly exposed land
- **Secondary Succession**: Recovery after disturbances
- **Climate-Driven Changes**: Shifts in biome boundaries due to climate changes
