#version 450

// Vertex Input
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in mat4 in_cube_model;

layout(location = 0) out vec2 frag_uv;

// [DEBUG] : Cube 0 model matrix: 
// 0.200000, 0.000000, 0.000000, 0.000000, 
// 0.000000, 0.200000, 0.000000, 0.000000, 
// 0.000000, 0.000000, 0.200000, 0.000000, 
// 0.000000, 0.000000, 0.000000, 1.000000
// [DEBUG] : Cube 1 model matrix: 
// 0.196940, 0.032246, -0.013227, 0.000000, 
// -0.026846, 0.188749, 0.060442, 0.000000, 
// 0.022228, -0.057742, 0.190189, 0.000000, 
// 2.000000, 5.000000, -15.000000, 1.000000
// [DEBUG] : Cube 2 model matrix: 
// 0.188128, 0.066004, -0.015858, 0.000000, 
// -0.045053, 0.156352, 0.116295, 0.000000, 
// 0.050776, -0.105819, 0.161939, 0.000000, 
// -1.500000, -2.200000, -2.500000, 1.000000
// [DEBUG] : Cube 3 model matrix: 
// 0.174627, 0.097201, -0.007574, 0.000000, 
// -0.052425, 0.106716, 0.160820, 0.000000, 
// 0.082201, -0.138432, 0.118657, 0.000000, 
// -3.800000, -2.000000, -12.300000, 1.000000
// [DEBUG] : Cube 4 model matrix: 
// 0.158066, 0.122075, 0.010623, 0.000000, 
// -0.048074, 0.045830, 0.188649, 0.000000, 
// 0.112713, -0.151648, 0.065564, 0.000000, 
// 2.400000, -0.400000, -3.500000, 1.000000
// [DEBUG] : Cube 5 model matrix: 
// 0.140442, 0.137626, 0.036541, 0.000000, 
// -0.032523, -0.018964, 0.196425, 0.000000, 
// 0.138630, -0.143873, 0.009063, 0.000000, 
// -1.700000, 3.000000, -7.500000, 1.000000
// [DEBUG] : Cube 6 model matrix: 
// 0.123881, 0.141977, 0.067052, 0.000000, 
// -0.007649, -0.079851, 0.183208, 0.000000, 
// 0.156828, -0.116044, -0.044030, 0.000000, 
// 1.300000, -2.000000, -2.500000, 1.000000
// [DEBUG] : Cube 7 model matrix: 
// 0.110380, 0.134605, 0.098477, 0.000000, 
// 0.023548, -0.129486, 0.150595, 0.000000, 
// 0.165111, -0.071518, -0.087312, 0.000000, 
// 1.500000, 2.000000, -2.500000, 1.000000
// [DEBUG] : Cube 8 model matrix: 
// 0.101568, 0.116398, 0.127026, 0.000000, 
// 0.057306, -0.161883, 0.102518, 0.000000, 
// 0.162481, -0.015666, -0.115562, 0.000000, 
// 1.500000, 0.200000, -1.500000, 1.000000
// [DEBUG] : Cube 9 model matrix: 
// 0.098507, 0.089552, 0.149254, 0.000000, 
// 0.089552, -0.173134, 0.044776, 0.000000, 
// 0.149254, 0.044776, -0.125373, 0.000000, 
// -1.300000, 1.000000, -1.500000, 1.000000

layout(set = 0, binding = 0) uniform Camera {
    mat4 view;
    mat4 projection;
} camera;


void main() {
    // mat4 i = mat4(0.200000, 0.000000, 0.000000, 0.000000, 
    //               0.000000, 0.200000, 0.000000, 0.000000, 
    //               0.000000, 0.000000, 0.200000, 0.000000, 
    //               0.000000, 0.000000, 0.000000, 1.000000);
    gl_Position = camera.projection * camera.view * in_cube_model * vec4(in_position, 1.0);
    // gl_Position = camera.projection * camera.view * i * vec4(in_position, 1.0);
    frag_uv = in_uv;
}