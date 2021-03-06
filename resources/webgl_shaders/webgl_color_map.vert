precision highp float;

uniform vec2 offset;
uniform vec2 scale;

attribute vec2 in_pos;
varying vec2 uv;

void main() {
    vec2 scaled_vertex = vec2(in_pos.x * scale.x, in_pos.y * scale.y) + offset;
    gl_Position  = vec4(2.0*scaled_vertex.x - 1.0,
                        1.0 - 2.0*scaled_vertex.y,
                        0.0, 1.0);
	uv = in_pos;
}