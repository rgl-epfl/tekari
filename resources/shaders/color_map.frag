#version 330

uniform sampler2D color_map;
in vec2 uv;
out vec4 color;

void main() {
    color = texture(color_map, uv);
}