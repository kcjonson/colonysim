#version 330 core
in vec3 Color;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform vec3 lightDir;
uniform vec3 lightColor;

void main() {
    // Ambient lighting
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Combine lighting with vertex color
    vec3 result = (ambient + diffuse) * Color;
    FragColor = vec4(result, 1.0);
}