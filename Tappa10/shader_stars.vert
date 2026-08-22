#version 410 core

layout (location = 0) in vec3 aDir; //Direzione unitaria sull'emisfero, generata una volta in Sky::BuildStars

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform float starRadius;
uniform float pointSize;

void main(){
    vec3 worldPos = cameraPos + aDir * starRadius;
    gl_Position = projection * view * vec4(worldPos, 1.0);
    gl_PointSize = pointSize;
}
