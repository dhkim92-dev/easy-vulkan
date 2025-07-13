#version 450

// Vertex Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 3) in mat4 in_cube_model;

layout(location = 0) out vec3 frag_pos;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec2 frag_uv;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec4 view_pos;
} camera;

void main() {
    frag_pos = vec3(in_cube_model * vec4(in_position, 1.0));
    frag_normal = mat3(transpose(inverse(in_cube_model))) * in_normal; // 변형된 평면에서의 법선 벡터로 변환 
    frag_uv = in_uv;

    gl_Position = camera.proj * camera.view * in_cube_model * vec4(in_position, 1.0);
}