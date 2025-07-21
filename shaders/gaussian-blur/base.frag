#version 450

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D texture_sampler;

void main() {
    vec4 color = texture(texture_sampler, frag_uv);
    out_color = vec4(color.rgb, 1.0);
}
