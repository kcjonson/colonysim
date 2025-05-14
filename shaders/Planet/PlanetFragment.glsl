#version 330 core

in vec3 Normal;
in vec3 FragPos;
in vec3 VertexColor;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 planetColor;
uniform int useColorAttrib = 0; // Whether to use the vertex color attribute (0=false, 1=true)

void main() {
    // Select the color source based on useColorAttrib uniform
    // If 1, use vertex color. Otherwise use planetColor or default
    vec3 baseColor;
    
    if (useColorAttrib == 1) {
        // Use the color from vertex attributes
        baseColor = VertexColor;
    } else {
        // Use planetColor if provided, otherwise use default ocean color
        baseColor = planetColor.x + planetColor.y + planetColor.z > 0.0 ? planetColor : vec3(0.0, 0.3, 0.8);
    }
    
    // Ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * baseColor;
    FragColor = vec4(result, 1.0);
}