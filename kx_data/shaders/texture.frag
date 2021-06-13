#version 410 core

in vec2 tex_coord;
in vec4 color_mod;

out vec4 frag_color;

uniform sampler2D texture_in;
uniform int conversion_algo;

float srgb_to_linear(float v)
{
    if(v <= 0.04045)
        return v / 12.92;
    else
        return pow((v+0.055)/1.055, 2.4);
}

float linear_to_srgb(float v)
{
    if(v <= 0.0031308)
        return v * 12.92;
    else
        return 1.055 * pow(v, 1.0 / 2.4) - 0.055;
}

void main()
{
    vec4 c = texture(texture_in, tex_coord);

    if(conversion_algo == -1)
        c = vec4(srgb_to_linear(c.r), srgb_to_linear(c.g), srgb_to_linear(c.b), c.a);
    else if(conversion_algo == 1)
        c = vec4(linear_to_srgb(c.r), linear_to_srgb(c.g), linear_to_srgb(c.b), c.a);

    frag_color = c * color_mod;
}
