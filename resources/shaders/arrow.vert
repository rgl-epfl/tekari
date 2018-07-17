#version 150 core

in vec4 in_pos;

uniform mat4 model_view_proj;

void main()
{
    gl_Position = model_view_proj * in_pos;
}