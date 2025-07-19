#version 450

layout(location = 0) in vec2 frag_uv;
layout (set = 0, binding = 0) uniform sampler2D cube_sampler;

layout(location = 0) out vec4 frag_color;

void main() {
    frag_color = texture(cube_sampler, frag_uv); // Sample the texture at the given UV coordinates
}