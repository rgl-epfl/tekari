#version 150 core

layout(points) in;
layout(line_strip, max_vertices = 2) out;

uniform mat4 modelViewProj;

void main()
{
  gl_Position = gl_in[0].gl_Position;
  EmitVertex();
  gl_Position = gl_in[0].gl_Position + modelViewProj * vec4(0, 1, 0, 0);
  EmitVertex();
  EndPrimitive();
}