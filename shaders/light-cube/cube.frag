#version 450
// inputs
layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

// outputs
layout(location = 0) out vec4 out_color;

// uniforms
layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
    vec4 view_pos;
} camera;
layout(set = 0, binding = 1) uniform sampler2D texture_sampler; // Texture sampler
layout(set = 0, binding = 2) uniform Light {
    vec4 pos;
    mat4 model;
} light;


void main() {
    vec3 light_ambient = vec3(0.2, 0.2, 0.2); // Ambient light
    vec3 light_diffuse = vec3(0.5, 0.5, 0.5); // Diffuse light
    vec3 light_specular = vec3(1.0, 1.0, 1.0); // Specular light

    vec3 material_specular = vec3(0.5, 0.5, 0.5); // Material specular color
    float material_shiness = 64.0;
    
    // ambient
    vec3 ambient = light_ambient * texture(texture_sampler, in_uv).rgb;

    // diffuse
    vec3 normal = normalize(in_normal);
    vec3 light_dir = normalize(light.pos.xyz - in_pos);
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = light_diffuse * diff * texture(texture_sampler, in_uv).rgb;

    // specular
    vec3 view_dir = normalize(camera.view_pos.xyz - in_pos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material_shiness);
    vec3 specular = light_specular * spec * texture(texture_sampler, in_uv).rgb;

    vec3 result = ambient + diffuse + specular;
    out_color = vec4(result, 1.0);
}