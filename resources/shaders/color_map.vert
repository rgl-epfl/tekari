#version 330

uniform vec2 offset;
uniform vec2 scale;

in vec2 in_pos;
out vec2 uv;

void main() {
    vec2 scaledVertex = vec2(in_pos.x * scale.x, in_pos.y * scale.y) + offset;
    gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                        1.0 - 2.0*scaledVertex.y,
                        0.0, 1.0);
	uv = in_pos;
}