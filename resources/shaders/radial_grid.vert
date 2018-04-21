#version 330

uniform mat4 modelViewProj;

in vec3 in_pos;

void main() {
    gl_Position = modelViewProj * vec4(in_pos, 1.0);
}