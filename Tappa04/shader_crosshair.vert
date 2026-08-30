#version 410 core

layout (location = 0) in vec2 aPos;

uniform float aspectRatio;

void main(){
    vec2 pos = aPos;
    pos.x /= aspectRatio; //Corregge la distorsione dovuta al rapporto larghezza/altezza
    gl_Position = vec4(pos, 0.0, 1.0);
}