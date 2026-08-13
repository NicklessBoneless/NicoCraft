#version 410 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 uv;

uniform mat4 model;
uniform mat4 vp;

out vec2 interpolated_uv;

void main()
{
    gl_Position = vp * model * vec4 (pos, 1.0);
    interpolated_uv = uv;
}