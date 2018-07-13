#version 150 core
in vec3 pos;

uniform vec3 origin;
uniform mat4 model_view_proj;

void main()
{
    gl_Position = model_view_proj * vec4(pos + origin, 1.0);
}