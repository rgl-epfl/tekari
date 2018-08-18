precision highp float;

uniform mat4 model_view_proj;
uniform mat4 model;
uniform mat4 inverse_transpose_model;

attribute vec2 in_pos2d;
attribute float in_height;
attribute vec4 in_normal;
attribute vec3 in_color;

varying float height;
varying vec3 position;
varying vec3 normal;
varying vec3 integrated_color;

void main() {
    vec4 pos = vec4(in_pos2d, in_height, 1.0);
    gl_Position = model_view_proj * pos;
    height = in_height;
    position = (model * pos).xyz;
    normal = (inverse_transpose_model * in_normal).xyz;
    integrated_color = in_color;
}