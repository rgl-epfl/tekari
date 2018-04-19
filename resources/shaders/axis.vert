#version 150 core
in vec3 pos;

uniform mat4 modelViewProj;
uniform vec3 origin;

void main()
{
    gl_Position = modelViewProj * vec4(pos + origin, 1.0);
}