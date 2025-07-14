#version 450

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 frag_pos;
layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform UBO {
    mat4 view;
    mat4 proj;
    vec4 light_pos; 
} ubo;

void main() {
     vec3 base_color = vec3(0.9, 0.05, 0.05);  // 강한 붉은색
    vec3 light_color = vec3(1.0);             // 흰색 광원

    vec3 normal = normalize(in_normal);
    vec3 light_dir = normalize(ubo.light_pos.xyz - frag_pos);

    // 카메라 위치 추출
    vec3 view_pos = vec3(inverse(ubo.view)[3]);
    vec3 view_dir = normalize(view_pos - frag_pos);

    // ambient
    float ambient_strength = 0.2;
    vec3 ambient = ambient_strength * light_color;

    // diffuse
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * light_color;

    // specular
    float specular_strength = 256.0;;
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 64.0);
    vec3 specular = specular_strength * spec * light_color;

    vec3 result = (ambient + diffuse + specular) * base_color;
    out_color = vec4(result, 1.0);
}