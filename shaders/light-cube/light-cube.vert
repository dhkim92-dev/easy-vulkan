#version 450

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec3 view_pos;
} camera;

layout(set = 0, binding = 1) uniform Light {
    vec4 pos;
    mat4 model;
} light;

void main() {
    gl_Position = camera.proj * 
                  camera.view * 
                  light.model *
                  vec4(in_pos, 1.0);
}