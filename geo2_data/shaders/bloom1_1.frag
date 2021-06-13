#version 410 core

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D texture_in;

void main()
{
    vec4 c = texture(texture_in, tex_coord);
    float luminance = 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    if(luminance > 1)
        frag_color = vec4(c.rgb * (luminance - 1) / luminance, c.a);
    else frag_color = vec4(0);
}
