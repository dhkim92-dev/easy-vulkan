#version 450
layout(binding = 1) uniform sampler2D texture_sampler; // Texture sampler
layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

void main() {
    out_color = texture(texture_sampler, in_uv); // Sample the texture at the given UV coordinates
}