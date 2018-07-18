precision highp float;

uniform vec3 view;
uniform bool use_shadows;
uniform bool use_specular;

uniform sampler2D color_map;

varying float height;
varying vec3 position;
varying vec3 normal;

const vec3 light_color = vec3(0.8, 0.8, 0.7);

void main() {
	vec3 color = texture2D(color_map, vec2(height, 0)).rgb;
	
	vec3 pos_to_light = normalize(view - position);
	float specular = abs(dot(reflect(-pos_to_light, normal), pos_to_light));
	vec3 out_color3 = vec3(0.0);
	out_color3 += float(use_shadows) * (0.2 + abs(dot(pos_to_light, normal))) * color * light_color;
	out_color3 += float(use_shadows && use_specular && specular > 0.0) * pow(specular, 20.0) * light_color;
	out_color3 += float(!use_shadows) * color;

	gl_FragColor = vec4(out_color3, 1.0);
}