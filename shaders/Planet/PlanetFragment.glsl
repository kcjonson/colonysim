#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec3 TileData; // x=terrainType, y=plateId, z=elevation

out vec4 FragColor;

// Lighting uniforms
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// Visualization mode
uniform int visualizationMode; // 0=terrain, 1=plates, 2=crust, 3=mesh
uniform bool isArrow; // Flag to indicate arrow rendering

// Color lookup tables
uniform vec3 terrainColors[16];
uniform vec3 plateColors[32];

void main() {
    // Check if this is arrow rendering
    if (isArrow) {
        // Simple bright color for all arrows - highly visible
        vec3 arrowColor = vec3(1.0, 1.0, 0.0); // Bright yellow
        FragColor = vec4(arrowColor, 1.0);
        return;
    }
    
    // Extract tile data
    int terrainType = int(TileData.x);
    int plateId = int(TileData.y);
    float elevation = TileData.z;
    
    // Determine base color based on visualization mode
    vec3 baseColor;
    
    if (visualizationMode == 0) { // Terrain
        baseColor = terrainColors[clamp(terrainType, 0, 15)];
    } 
    else if (visualizationMode == 1) { // Tectonic plates
        if (plateId >= 0 && plateId < 32) {
            baseColor = plateColors[plateId];
        } else {
            baseColor = vec3(0.5, 0.5, 0.5); // Gray for unassigned
        }
    }
    else if (visualizationMode == 2) { // Crust thickness (elevation-based)
        float thickness = (elevation + 1.0) * 0.5; // Normalize to 0-1
        baseColor = vec3(thickness, thickness * 0.5, 0.2);
    }
    else if (visualizationMode == 3) { // Mesh
        baseColor = vec3(0.7, 0.7, 0.7);
    }
    else {
        baseColor = vec3(1.0, 0.0, 1.0); // Magenta for unknown mode
    }
    
    // Lighting calculation
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = (ambient + diffuse) * baseColor;
    FragColor = vec4(result, 1.0);
}