#version 410 core

layout (location = 0) in vec3 starCoordinates;
layout (location = 1) in vec2 quadCorners;
layout (location = 2) in float starRotation;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform float starRadius;
uniform float starSize;

void main(){
    //Ruota il corner locale (-1,-1)/(1,-1)/(1,1)/(-1,1) dell'angolo random della stella
    float c = cos(starRotation);
    float s = sin(starRotation);
    vec2 rotatedCornersXY = vec2(quadCorners.x * c - quadCorners.y * s, quadCorners.x * s + quadCorners.y * c);

    //Assi camera estratti dalla view matrix, per il billboard verso lo schermo
    vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 cameraUp    = vec3(view[0][1], view[1][1], view[2][1]);

    //Spostiamo le stelle assieme alla camera in modo che dia l'illusione che siano in alto nel cielo
    vec3 worldStarPos = cameraPos + starCoordinates * starRadius + 
        (rotatedCornersXY.x * cameraRight + rotatedCornersXY.y * cameraUp) * starSize;

    gl_Position = projection * view * vec4(worldStarPos, 1.0);
}
