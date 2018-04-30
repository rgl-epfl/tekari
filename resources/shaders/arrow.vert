#version 150 core
in vec3 pos;

uniform vec3 origin;
uniform mat4 modelViewProj;

void main()
{
    gl_Position = modelViewProj * vec4(pos + origin, 1.0);
}