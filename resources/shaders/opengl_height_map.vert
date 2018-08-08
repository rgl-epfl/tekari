#version 330

uniform mat4 model_view_proj;
uniform mat4 model;
uniform mat4 inverse_transpose_model;

in vec4 in_normal;
in vec2 in_pos2d;
in float in_height;

out float height;
out vec3 position;
out vec3 normal;

void main() {
    vec4 pos = vec4(in_pos2d, in_height, 1.0);
    gl_Position = model_view_proj * pos;
    height = in_height;
    position = (model * pos).xyz;
    normal = (inverse_transpose_model * in_normal).xyz;
}