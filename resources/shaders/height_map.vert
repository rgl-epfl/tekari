#version 330

uniform mat4 model_view_proj;
uniform mat4 model;

in vec4 in_normal;
in vec2 in_pos2d;
in float in_height;
in int in_selected;

out float height;
out vec3 position;
out vec3 normal;

void main() {
    vec3 pos = vec3(in_pos2d.x, in_height, in_pos2d.y);
    gl_Position = model_view_proj * vec4(pos, 1.0);
    height = in_height;
    position = (model * vec4(pos, 1.0)).xyz;
    normal = (transpose(inverse(model)) * in_normal).xyz;
}