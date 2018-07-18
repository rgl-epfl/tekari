precision highp float;

uniform bool show_all_points;
uniform mat4 model_view_proj;
uniform sampler2D color_map;

attribute vec2 in_pos2d;
attribute float in_height;
// attribute int in_selected;

varying vec4 color;

void main() {
	float selected = 0.0;//float(in_selected != 0);
	float show_all_points_f = float(show_all_points);
	color = mix(mix(vec4(0.0), vec4(vec3(1.0) - texture2D(color_map, vec2(in_height, 0.0)).rgb, 1.0), show_all_points_f),
				vec4(0.0, 0.8, 0.3, 1.0),
				selected);
    gl_Position = model_view_proj * vec4(in_pos2d.x, in_height, in_pos2d.y, 1.0);
}