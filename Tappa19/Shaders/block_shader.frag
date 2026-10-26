#version 410 core

in vec2 outUVCoordinates;
in float outTexture; //Passato dal Vertex Shader
in float outBrightness; //Passato dal Vertex Shader
in float outFogDistance;

out vec4 FragColor;

uniform vec3 fogColor;
uniform float fogStart;
uniform float fogEnd;
uniform sampler2DArray textureArray;
uniform float daylightFactor; //Fattore globale del ciclo giorno/notte, aggiornato da Renderer::Draw

void main()
{
    vec4 textureColor = texture(textureArray, vec3(outUVCoordinates, outTexture));
    if(textureColor.a < 0.5){
        discard;
    }
    textureColor.rgb *= outBrightness * daylightFactor;

    //Nebbia: Sfuma verso il colore del cielo, mascherando il bordo del far plane
    float fogFactor = clamp((outFogDistance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    textureColor.rgb = mix(textureColor.rgb, fogColor, fogFactor);

    FragColor = textureColor;
}
