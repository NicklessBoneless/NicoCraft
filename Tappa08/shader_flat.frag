#version 410 core

in vec2 TexCoord;
in float TexLayer; //Passato dal Vertex Shader

out vec4 FragColor;

uniform sampler2DArray textureArray;

void main()
{
    // Campiona dalla Texture Array usando UV (2D) + Layer (1D)
    FragColor = texture(textureArray, vec3(TexCoord, TexLayer));
}