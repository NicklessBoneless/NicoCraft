#version 410 core

layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 UVCoordinates;

out vec2 outUVCoordinates;

uniform vec2 screenSize; //Dimensioni finestra in pixel

void main(){
    //Da pixel (origine in alto a sinistra) a NDC
    vec2 ndc = vec2(vertexPosition.x / screenSize.x * 2.0 - 1.0, 1.0 - vertexPosition.y / screenSize.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    outUVCoordinates = UVCoordinates;
}
