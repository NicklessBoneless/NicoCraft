#version 410 core

in vec2 outUVCoordinates;

out vec4 FragColor;

uniform sampler2D celestialTexture;
uniform float alpha;

void main(){
    vec4 textureColor = texture(celestialTexture, outUVCoordinates);
    textureColor.a *= alpha;

    if(textureColor.a < 0.01){
        discard;
    }

    FragColor = textureColor;
}
