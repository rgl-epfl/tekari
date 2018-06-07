#version 330

uniform bool showAllPoints;
uniform mat4 modelViewProj;
uniform sampler1D color_map;

in vec2 in_pos2d;
in float in_height;
in int in_selected;

out vec4 color;

void main() {
	float selected = float(in_selected != 0);
	float showAllPointsF = float(showAllPoints);
	color = mix(mix(vec4(0.0f), vec4(vec3(1.0f) - texture(color_map, in_height).rgb, 1.0f), showAllPointsF),
				vec4(0.0f, 0.8f, 0.3f, 1.0f),
				selected);
    gl_Position = modelViewProj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}