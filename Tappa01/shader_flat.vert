#version 410 core

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 uvCoordinates;

uniform mat4 model;
uniform mat4 vp;

out vec2 interpolated_uv;

void main()
{
    gl_Position = vp * model * vec4 (VertexPosition, 1.0);
    interpolated_uv = uvCoordinates;
}