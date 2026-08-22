#version 410 core

in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D celestialTexture;
uniform float alpha;

void main(){
    vec4 texColor = texture(celestialTexture, TexCoord);
    texColor.a *= alpha;

    if(texColor.a < 0.01){
        discard;
    }

    FragColor = texColor;
}
