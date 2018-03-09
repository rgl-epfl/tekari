#version 330

uniform vec3 view;

in float height;
in vec3 position;
in vec3 normal;

out vec4 out_color;

void main() {
    float brightness = abs(dot(normalize(view - position), normal));
    vec3 color = vec3(height, 1 - abs((height - 0.5)*2), 1-height);

    out_color = vec4((0.2 + brightness*.8) * color, 1.0f);
}