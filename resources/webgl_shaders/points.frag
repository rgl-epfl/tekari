precision highp float;

varying vec4 color;

void main() {
	if (color.a == 0.0)
		discard;
    gl_FragColor = color;
}