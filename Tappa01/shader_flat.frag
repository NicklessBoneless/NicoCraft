#version 410 core

uniform sampler2D block_texture;

// luce di base fissa: nessun calcolo dinamico, solo un fattore di illuminazione costante
const float ambient_light = 0.9;

in vec2 interpolated_uv;

out vec4 fragment_color;

void main()
{
    vec3 tex_color = texture (block_texture, interpolated_uv).rgb;
    fragment_color = vec4 (tex_color * ambient_light, 1.0);
}