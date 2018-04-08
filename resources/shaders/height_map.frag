#version 330

uniform vec3 view;
uniform bool useShadows;

uniform sampler1D color_map;

in float height;
in vec3 position;
in vec3 normal;

out vec4 out_color;

void main() {
	vec3 color = texture(color_map, height).rgb;
	if (useShadows)
	{
	    float brightness = abs(dot(normalize(view - position), normal));
	    out_color = vec4((0.2 + brightness*.8) * color, 1.0f);
	}
	else
	{
		out_color = vec4(color, 1.0f);
	}
}