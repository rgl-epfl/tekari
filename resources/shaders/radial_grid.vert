#version 330

uniform mat4 model_view_proj;

in vec4 in_pos;

void main() {
    gl_Position = model_view_proj * in_pos;
}