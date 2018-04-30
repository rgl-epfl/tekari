#version 150 core

layout(points) in;
layout(triangle_strip, max_vertices = 132) out;

uniform vec3 direction;
uniform float length;
uniform mat4 modelViewProj;

const float PI = 3.1415926;

const float lineRadius = 0.002f;
const float coneRadius = 0.01f;
const float coneHeight = 0.05f;

void drawArrow(vec4 front, vec4 up, vec4 right)
{
  vec4 cylingerBaseCenter1 = gl_in[0].gl_Position;
  vec4 cylingerBaseCenter2 = gl_in[0].gl_Position + modelViewProj * length * front;
  vec4 coneBaseCenter = gl_in[0].gl_Position + modelViewProj * length * front;
  vec4 coneTip        = gl_in[0].gl_Position + modelViewProj * (length + coneHeight) * front;

  // draw cylinder for line
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

  // draw cone for arrow
  for(int i = 0; i <= 10; ++i)
  {
    float angle = PI * 2.0 / 10.0 * i;
    vec4 offset = modelViewProj * (sin(angle)*up + cos(angle)*right) * coneRadius;
    gl_Position = coneBaseCenter + offset;
    EmitVertex();
    gl_Position = coneTip;
    EmitVertex();
  }
  EndPrimitive();
}

void main()
{
  vec3 up = vec3(0, 1, 0);
  if (direction == up)
  {
    up = vec3(1, 0, 0);
  }
  vec3 right = normalize(cross(direction, up));
  up = normalize(cross(right, direction));
  drawArrow(vec4(direction, 0), vec4(up, 0), vec4(right, 0));
}