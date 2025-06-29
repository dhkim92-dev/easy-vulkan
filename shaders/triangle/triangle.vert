#version 450

layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor; // Vertex color
layout(location = 0) out vec3 fragColor; // Output color to fragment shader

// UBO 선언
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    // gl_Position = vec4(inPosition, 1.0); // NDC에 바로 위치, 항상 보임
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0); // Set the position of the vertex
    fragColor = inColor; // Pass the color to the fragment shader
    // fragColor = ubo.view[0].xyz;
}
