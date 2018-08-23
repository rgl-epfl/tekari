#version 330

uniform bool use_diffuse_shading;
uniform bool use_specular_shading;
uniform bool use_integrated_colors;

uniform sampler2D color_map;

in float height;
in vec3 position;
in vec3 normal;
in vec3 integrated_color;

out vec4 out_color;

const vec3 light_color = vec3(0.8f, 0.8f, 0.7f);

void main() {
	vec3 color = use_integrated_colors ? integrated_color : texture(color_map, vec2(height, 0)).rgb;

	vec3 pos_to_light = normalize(vec3(0, 0, 4) - position);
	float specular = clamp(dot(reflect(-pos_to_light, normal), pos_to_light), 0.0f, 1.0f);
	vec3 out_color3 = vec3(0.0f);
	out_color3 += float(use_diffuse_shading) * (0.2f + abs(dot(pos_to_light, normal))) * color * light_color;
	out_color3 += float(use_diffuse_shading && use_specular_shading) * pow(specular, 20.0f) * light_color;
	out_color3 += float(!use_diffuse_shading) * color;

    out_color = vec4(out_color3, 1.0f);
}