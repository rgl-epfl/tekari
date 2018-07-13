#version 330

uniform bool show_all_points;
uniform mat4 model_view_proj;
uniform sampler1D color_map;

in vec2 in_pos2d;
in float in_height;
in int in_selected;

out vec4 color;

void main() {
	float selected = float(in_selected != 0);
	float show_all_points_f = float(show_all_points);
	color = mix(mix(vec4(0.0f), vec4(vec3(1.0f) - texture(color_map, in_height).rgb, 1.0f), show_all_points_f),
				vec4(0.0f, 0.8f, 0.3f, 1.0f),
				selected);
    gl_Position = model_view_proj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}