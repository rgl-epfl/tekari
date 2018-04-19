#version 150 core

layout(points) in;
layout(triangle_strip, max_vertices = 132) out;

uniform mat4 modelViewProj;
out vec3 fcolor;

const float PI = 3.1415926;

const float lineRadius = 0.002f;
const float lineLength = 0.15f;
const float baseRadius = 0.01f;
const float coneHeight = 0.05f;

void drawArrow(vec4 front, vec4 up, vec4 right, vec3 color)
{
  fcolor = color;
  vec4 cylingerBaseCenter1 = gl_in[0].gl_Position;
  vec4 cylingerBaseCenter2 = gl_in[0].gl_Position + modelViewProj * lineLength * front;

  for(int i = 0; i <= 10; ++i)
  {
    float angle = PI * 2.0 / 10.0 * i;
    vec4 offset = modelViewProj * (sin(angle)*up + cos(angle)*right) * lineRadius;
    gl_Position = cylingerBaseCenter1 + offset;
    EmitVertex();
    gl_Position = cylingerBaseCenter2 + offset;
    EmitVertex();
  }
  EndPrimitive();

  vec4 coneBaseCenter = gl_in[0].gl_Position + modelViewProj * lineLength * front;
  vec4 coneTip        = gl_in[0].gl_Position + modelViewProj * (lineLength + coneHeight) * front;
  for(int i = 0; i <= 10; ++i)
  {
    float angle = PI * 2.0 / 10.0 * i;
    vec4 offset = modelViewProj * (sin(angle)*up + cos(angle)*right) * baseRadius;
    gl_Position = coneBaseCenter + offset;
    EmitVertex();
    gl_Position = coneTip;
    EmitVertex();
  }
  EndPrimitive();
}

void main()
{
// draw axis lines
  drawArrow(vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec3(1, 0, 0));
  drawArrow(vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(1, 0, 0, 0), vec3(0, 1, 0));
  drawArrow(vec4(0, 0, 1, 0), vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec3(0, 0, 1));
}