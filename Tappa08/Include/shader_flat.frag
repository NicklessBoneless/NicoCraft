#version 410 core

in vec2 outUvCoordinates;
in float outTextureIndex; //Passato dal Vertex Shader

out vec4 FragColor;

uniform sampler2DArray textureArray;

void main()
{
    vec4 texColor = texture(textureArray, vec3(outUvCoordinates, outTextureIndex));

    //Alpha test: scarta i pixel trasparenti invece di disegnarli neri.
    //Niente blending necessario: la trasparenza delle foglie e' binaria (o c'e' o non c'e')
    if(texColor.a < 0.5){
        discard;
    }

    FragColor = texColor;
}
