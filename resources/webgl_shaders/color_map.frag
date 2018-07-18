precision highp float;

uniform sampler2D color_map;
varying vec2 uv;

void main() {
    gl_FragColor = texture2D(color_map, uv);
}