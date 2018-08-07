#version 330

uniform bool show_all_points;
uniform mat4 model_view_proj;
uniform sampler2D color_map;

in vec2 in_pos2d;
in float in_height;
in float in_selected;

out vec4 color;

void main() {
	float show_all_points_f = float(show_all_points);
	color = mix(mix(vec4(0.0f), vec4(vec3(1.0f) - texture(color_map, vec2(in_height, 0.0)).rgb, 1.0f), show_all_points_f),
				vec4(0.0f, 0.8f, 0.3f, 1.0f),
				in_selected);
    gl_Position = model_view_proj * vec4(in_pos2d, in_height, 1.0);
}