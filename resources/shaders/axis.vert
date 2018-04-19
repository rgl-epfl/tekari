#version 150 core
in vec3 pos;

uniform mat4 modelViewProj;

void main()
{
    gl_Position = modelViewProj * vec4(pos, 1.0);
}