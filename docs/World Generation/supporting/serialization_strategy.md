# Serialization Strategy for World Generation System

## 1. Overview

This document outlines the serialization strategy for the world generation system, enabling users to save and load generated worlds. The strategy focuses on efficient storage, backward compatibility, and seamless integration with both the standalone world generator and the main game.

## 2. File Format Design

### 2.1 File Structure

The world data will be stored in a custom binary format with the following structure:

```
[Header]
- Magic Number (4 bytes): "WGEN"
- Version (4 bytes): Major (2 bytes) + Minor (2 bytes)
- Timestamp (8 bytes): Creation time
- Checksum (4 bytes): CRC32 of data section

[Metadata]
- Name Length (2 bytes)
- World Name (variable)
- Parameter Count (2 bytes)
- Parameters (variable key-value pairs)

[Planet Parameters]
- Serialized PlanetParameters struct (fixed size)

[Compressed Data Sections]
- Section 1: Elevation Data
  - Size (4 bytes)
  - Compressed Data (variable)
- Section 2: Temperature Data
  - Size (4 bytes)
  - Compressed Data (variable)
- Section 3: Precipitation Data
  - Size (4 bytes)
  - Compressed Data (variable)
- Section 4: Biome Data
  - Size (4 bytes)
  - Compressed Data (variable)
- Section 5: Feature Data (tectonic plates, rivers, etc.)
  - Size (4 bytes)
  - Compressed Data (variable)
```

### 2.2 Compression Strategy

To minimize file size while maintaining performance:

1. **Grid Data Compression**:
   - Use zlib compression for grid data (elevation, temperature, etc.)
   - Quantize floating-point values to reduce size before compression
   - Store data in row-major order for better compression ratios

2. **Feature Data Compression**:
   - Serialize complex objects (rivers, plates) to binary format
   - Compress the serialized data with zlib
   - Use delta encoding for sequences of similar values

### 2.3 Versioning and Compatibility

To ensure backward compatibility:

1. **Version Checking**:
   - Check version number during loading
   - Implement conversion logic for older versions

2. **Extensible Format**:
   - Optional sections can be added in newer versions
   - Unknown sections are skipped by older versions

3. **Migration Path**:
   - Include utility functions to upgrade older save formats
   - Document format changes between versions

## 3. Implementation Details

### 3.1 Serialization Class

The `Serialization` class will provide the following functionality:

```cpp
// Save planet parameters to file
bool SaveParameters(const PlanetParameters& parameters, const std::string& filename);

// Load planet parameters from file
bool LoadParameters(const std::string& filename, PlanetParameters& parameters);

// Save complete planet data to file
bool SavePlanetData(const std::shared_ptr<PlanetData>& planetData, const std::string& filename);

// Load complete planet data from file
std::shared_ptr<PlanetData> LoadPlanetData(const std::string& filename);

// Get list of saved worlds
std::vector<std::string> GetSavedWorldsList(const std::string& directory);

// Get metadata for a saved world
std::unordered_map<std::string, std::string> GetWorldMetadata(const std::string& filename);
```

### 3.2 Data Conversion

For efficient serialization and deserialization:

1. **Binary Serialization**:
   - Direct memory mapping for simple structures
   - Custom serialization for complex objects

2. **Endianness Handling**:
   - Store data in little-endian format
   - Convert on load if running on big-endian systems

3. **Floating-Point Precision**:
   - Use appropriate precision for different data types
   - Elevation: 16-bit fixed point
   - Temperature: 8-bit scaled integer
   - Precipitation: 8-bit scaled integer

### 3.3 Game Integration

To transfer data to the main game:

1. **Conversion Function**:
   - `ConvertToGameFormat()` method in PlanetData
   - Generates a simplified representation for the game

2. **Loading Mechanism**:
   - Game loads the converted data instead of generating terrain
   - Existing World class modified to accept pre-generated data

## 4. File Management

### 4.1 Save Locations

The system will use the following directory structure:

```
/saves/
  /parameters/  - Saved parameter sets
  /worlds/      - Complete world saves
  /screenshots/ - Planet screenshots
```

### 4.2 Metadata and Indexing

Each save file will include metadata for easy identification:

- World name
- Creation date and time
- Planet parameters summary
- Thumbnail image data

A separate index file will maintain a catalog of all saved worlds for quick browsing.

### 4.3 Auto-Save Functionality

The system will implement auto-save features:

- Automatic saving after successful generation
- Temporary saves during generation process
- Recovery from crashes using auto-saved data

## 5. Performance Considerations

### 5.1 Memory Usage

To minimize memory impact:

- Stream data from disk when possible
- Load only required sections when viewing
- Unload unused data when switching visualization modes

### 5.2 Load/Save Times

To ensure responsive user experience:

- Background thread for save/load operations
- Progress indication for large world files
- Prioritize loading visible areas first

### 5.3 Storage Requirements

Estimated file sizes:

- Parameters only: ~1 KB
- Low-resolution world: ~5-10 MB
- High-resolution world: ~50-100 MB
- With compression: 70-80% reduction

## 6. Testing Strategy

To ensure reliability:

1. **Unit Tests**:
   - Test serialization/deserialization of all data types
   - Verify checksum validation

2. **Integration Tests**:
   - Save/load complete worlds of various sizes
   - Test backward compatibility with older versions

3. **Performance Tests**:
   - Measure load/save times for different world sizes
   - Monitor memory usage during operations
