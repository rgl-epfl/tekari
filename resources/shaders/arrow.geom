#version 150 core

layout(points) in;
layout(triangle_strip, max_vertices = 132) out;

uniform vec3 direction;
uniform float length;
uniform mat4 model_view_proj;

const float PI = 3.1415926;

const float line_radius = 0.002f;
const float cone_radius = 0.01f;
const float cone_height = 0.05f;

void draw_arrow(vec4 front, vec4 up, vec4 right)
{
  vec4 cylinger_base_center1 = gl_in[0].gl_Position;
  vec4 cylinger_base_center2 = gl_in[0].gl_Position + model_view_proj * length * front;
  vec4 cone_base_center = gl_in[0].gl_Position + model_view_proj * length * front;
  vec4 cone_tip        = gl_in[0].gl_Position + model_view_proj * (length + cone_height) * front;

  // draw cylinder for line
  for(int i = 0; i <= 10; ++i)
  {
    float angle = PI * 2.0 / 10.0 * i;
    vec4 offset = model_view_proj * (sin(angle)*up + cos(angle)*right) * line_radius;
    gl_Position = cylinger_base_center1 + offset;
    EmitVertex();
    gl_Position = cylinger_base_center2 + offset;
    EmitVertex();
  }
  EndPrimitive();

  // draw cone for arrow
  for(int i = 0; i <= 10; ++i)
  {
    float angle = PI * 2.0 / 10.0 * i;
    vec4 offset = model_view_proj * (sin(angle)*up + cos(angle)*right) * cone_radius;
    gl_Position = cone_base_center + offset;
    EmitVertex();
    gl_Position = cone_tip;
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
  draw_arrow(vec4(direction, 0), vec4(up, 0), vec4(right, 0));
}