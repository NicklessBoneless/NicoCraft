#version 410 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 UVCoordinates;

out vec2 outUVCoordinates;

uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = projection * view * vec4(VertexPosition, 1.0);
    outUVCoordinates = UVCoordinates;
}
