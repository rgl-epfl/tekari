#version 330

uniform mat4 modelViewProj;
uniform mat4 model;

in vec3 in_normal;
in vec3 in_position;

out float height;
out vec3 position;
out vec3 normal;

void main() {
    gl_Position = modelViewProj * vec4(in_position, 1.0);
    height = in_position.y;
    position = (model * vec4(in_position, 1.0)).xyz;
    normal = (model * vec4(in_normal, 0.0)).xyz;
}