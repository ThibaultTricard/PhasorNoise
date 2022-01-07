#version 450

layout( location = 0 ) in vec4 app_position;
layout( location = 1 ) in vec2 app_uv;

layout( location = 0 ) out vec2 vert_uv;

void main() {
  gl_Position = app_position;
  vert_uv = app_uv;
}