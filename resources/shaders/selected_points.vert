#version 330

uniform mat4 modelViewProj;

in vec2 in_pos2d;
in float in_height;
in int in_selected;

out vec3 color;

void main() {
	color = (in_selected != 0) ? vec3(0.0f, 0.8f, 0.3f) : vec3(1.0f);
    gl_Position = modelViewProj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}