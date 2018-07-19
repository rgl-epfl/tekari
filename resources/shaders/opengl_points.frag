#version 330

in vec4 color;
out vec4 out_color;

void main() {
	if (color.a == 0.0f)
		discard;
    out_color = color;
}