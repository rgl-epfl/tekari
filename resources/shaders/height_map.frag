#version 330

in float height;
in vec3 position;
in vec3 normal;
out vec4 color;
uniform vec3 view;

void main() {
    vec3 lightPos = view + vec3(0, 3, 0);
    float brightness = abs(dot(normalize(lightPos - position), normal));
    color = vec4(brightness * vec3(height, 1 - abs((height - 0.5)*2), 1-height), 1.0f);
}