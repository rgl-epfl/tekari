#version 330

uniform mat4 modelViewProj;

in vec2 in_pos;

void main() {
    gl_Position = modelViewProj * vec4(in_pos.x, 0.0f, in_pos.y, 1.0f);
}