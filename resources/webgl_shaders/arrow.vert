precision highp float;

uniform mat4 model_view_proj;
attribute vec4 in_pos;

void main() {
    gl_Position = model_view_proj * in_pos;
}