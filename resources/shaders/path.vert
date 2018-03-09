#version 330

uniform mat4 modelViewProj;

in vec2 in_pos2d;
in float in_height;

void main() {
    gl_Position = modelViewProj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}