precision highp float;

uniform mat4 model_view_proj;
uniform mat4 model;
uniform mat4 inverse_transpose_model;

attribute vec2 in_pos2d;
attribute float in_height;
attribute vec4 in_normal;

varying float height;
varying vec3 position;
varying vec3 normal;

void main() {
    vec3 pos = vec3(in_pos2d.x, in_height, in_pos2d.y);
    gl_Position = model_view_proj * vec4(pos, 1.0);
    height = in_height;
    position = (model * vec4(pos, 1.0)).xyz;
    normal = (inverse_transpose_model * in_normal).xyz;
}