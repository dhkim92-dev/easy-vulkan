#version 450

layout(location = 0) in vec2 in_uv;

layout(set = 1, binding = 0) uniform sampler2D texture_sampler;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 tex_color = texture(texture_sampler, in_uv);
    out_color = tex_color;
    // out_color = vec4(1.0, 0.0, 0.0, 1.0); // Output a solid red color for testing
}