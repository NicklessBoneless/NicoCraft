#version 410 core

in vec2 outUVCoordinates;
in float outTexture; //Passato dal Vertex Shader
in float outBrightness; //Passato dal Vertex Shader

out vec4 FragColor;

uniform sampler2DArray textureArray;
uniform float daylightFactor; //Fattore globale del ciclo giorno/notte, aggiornato da Renderer::Draw

void main()
{
    vec4 textureColor = texture(textureArray, vec3(outUVCoordinates, outTexture));
    /*
        Alpha test: scarta i pixel trasparenti invece di disegnarli neri. 
        Niente blending necessario: la trasparenza delle foglie e' binaria (o c'e' o non c'e')
    */
    if(textureColor.a < 0.5){
        discard;
    }
    textureColor.rgb *= outBrightness * daylightFactor; //Shading per faccia moltiplicato per la luce globale del momento
    FragColor = textureColor;
}
