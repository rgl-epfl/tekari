#version 330

uniform mat4 model_view_proj;

in vec2 in_pos;

void main() {
    gl_Position = model_view_proj * vec4(in_pos.x, in_pos.y, 0.0f, 1.0f);
}