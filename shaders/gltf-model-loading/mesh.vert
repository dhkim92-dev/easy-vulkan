#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(push_constant) uniform Push{
    mat4 model;
} push_constants;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) out vec2 out_uv;

void main() {
    out_uv = in_uv;
    // gl_Position = camera.proj * camera.view * mat4(1.0f) * vec4(in_pos, 1.0);
    gl_Position = camera.proj * camera.view * push_constants.model * vec4(in_pos, 1.0);
    // gl_Position = mat4(1.0k) * vec4(in_pos, 1.0);
}