#version 330

uniform vec3 view;
uniform bool use_shadows;
uniform bool use_specular;

uniform sampler1D color_map;

in float height;
in vec3 position;
in vec3 normal;

out vec4 out_color;

const vec3 light_color = vec3(0.8f, 0.8f, 0.7f);

void main() {
	vec3 color = texture(color_map, height).rgb;
	
	vec3 pos_to_light = normalize(view - position);
	float specular = abs(dot(reflect(-pos_to_light, normal), pos_to_light));
	vec3 out_color3 = vec3(0.0f);
	out_color3 += float(use_shadows) * (0.2f + abs(dot(pos_to_light, normal))) * color * light_color;
	out_color3 += float(use_shadows && use_specular && specular > 0.0f) * pow(specular, 20.0f) * light_color;
	out_color3 += float(!use_shadows) * color;

	out_color = vec4(out_color3, 1.0f);
}