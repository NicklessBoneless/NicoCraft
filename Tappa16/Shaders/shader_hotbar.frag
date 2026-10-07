#version 410 core

in vec2 outUVCoordinates;

out vec4 FragColor;

uniform sampler2D hotbarTexture;
uniform vec4 tint;

void main(){
    FragColor = texture(hotbarTexture, outUVCoordinates) * tint;
}
