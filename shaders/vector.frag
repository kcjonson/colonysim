#version 330 core
in vec4 Color;
out vec4 FragColor;
void main() {
    // Use the exact color and alpha as passed from the vertex shader
    // without any modifications or mixing
    FragColor = Color;
}