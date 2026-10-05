#version 410 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 UVCoordinates;
layout (location = 2) in float texture;
layout (location = 3) in float brightness;

out vec2 outUVCoordinates;
out float outTexture;
out float outBrightness;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
    outUVCoordinates = UVCoordinates;
    outTexture = texture;
    outBrightness = brightness;
}
