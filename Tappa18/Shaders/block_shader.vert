#version 410 core

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec2 UVCoordinates;
layout (location = 2) in float texture;
layout (location = 3) in float brightness;

out vec2 outUVCoordinates;
out float outTexture;
out float outBrightness;
out float outFogDistance;

uniform mat4 model;
uniform mat4 viewProjectionModel;

void main(){
    gl_Position = viewProjectionModel * vec4(vertexPosition, 1.0);
    outFogDistance = gl_Position.w;
    outUVCoordinates = UVCoordinates;
    outTexture = texture;
    outBrightness = brightness;
}
