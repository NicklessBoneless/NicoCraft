#version 410 core

in vec2 TexCoord;
in float TexLayer; //Passato dal Vertex Shader
in float Brightness; //Passato dal Vertex Shader

out vec4 FragColor;

uniform sampler2DArray textureArray;
uniform float daylightFactor; //Fattore globale del ciclo giorno/notte, aggiornato da Renderer::Draw

void main()
{
    vec4 texColor = texture(textureArray, vec3(TexCoord, TexLayer));

    //Alpha test: scarta i pixel trasparenti invece di disegnarli neri.
    //Niente blending necessario: la trasparenza delle foglie e' binaria (o c'e' o non c'e')
    if(texColor.a < 0.5){
        discard;
    }

    texColor.rgb *= Brightness * daylightFactor; //Shading per faccia moltiplicato per la luce globale del momento

    FragColor = texColor;
}
