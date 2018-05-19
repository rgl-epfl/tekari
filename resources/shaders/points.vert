#version 330

uniform bool showAllPoints;
uniform mat4 modelViewProj;
uniform sampler1D color_map;

in vec2 in_pos2d;
in float in_height;
in int in_selected;

out vec4 color;

void main() {
	vec3 inv_map_color = vec3(1.0f) - texture(color_map, in_height).rgb;
	color = in_selected != 0 ? vec4(0.0f, 0.8f, 0.3f, 1.0f) :
			!showAllPoints ? vec4(0.0f) :
			vec4(inv_map_color, 1.0f);
    gl_Position = modelViewProj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}