#version 450
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 0) out vec3 frag_normal;
layout(location = 1) out vec3 frag_pos;

layout(set = 0, binding = 0) uniform UBO {
    mat4 view; // Camera view matrix
    mat4 proj; // Projection matrix
    vec4 pos; // Light Position
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(in_position, 1.0);
    // gl_Position = vec4(in_position, 1.0);
    frag_normal = in_normal;
    frag_pos = in_position; // Pass the position to the fragment shader
}