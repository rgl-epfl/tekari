#version 330

uniform mat4 mvp;

in vec3 in_pos;

void main() {
    gl_Position = mvp * vec4(in_pos, 1.0);
}