precision highp float;

uniform mat4 model_view_proj;

attribute vec2 in_pos2d;
attribute float in_height;

void main() {
    gl_Position = model_view_proj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}