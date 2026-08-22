#version 410 core

layout (location = 0) in vec3 aPos;

uniform mat4 pvm; //Matrice da BlockOutline.hh

void main(){
    gl_Position = pvm * vec4(aPos, 1.0);
}
