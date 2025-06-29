#version 450

layout(location = 0) in vec3 inColor; // Vertex position

layout(location = 0) out vec4 outFlagColor;

void main() {
    outFlagColor = vec4(inColor, 1.0); // Set the fragment color to the input color with full opacity
    // The alpha value is set to 1.0 for full opacity
    // If you want transparency, you can set it to a value less than 1.0
}