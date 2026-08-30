#version 410 core

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec2 uvCoordinates;
layout (location = 2) in float textureIndex;

out vec2 outUvCoordinates;
out float outTextureIndex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = projection * view * model * vec4(VertexPosition, 1.0);
    outUvCoordinates = uvCoordinates;
    outTextureIndex = textureIndex;
}

