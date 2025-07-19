#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 frag_uv;

layout(push_constant) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    frag_uv = in_uv;
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos, 1.0);
}