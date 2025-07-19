#version 450

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D scene_texture;
layout(set = 0, binding = 1) uniform sampler2D outline_texture;

void main() {
    vec4 scene_color = texture(scene_texture, frag_uv);
    vec4 outline_color = texture(outline_texture, frag_uv);
    out_color = mix(scene_color, outline_color, 0.3);
}
