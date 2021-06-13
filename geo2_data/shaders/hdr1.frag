#version 410 core

in vec2 tex_coord;

out vec4 frag_color;

uniform sampler2D texture_in;

void main()
{
    vec3 c = texture(texture_in, tex_coord).xyz;
    float luminance = 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
    c /= 1 + c;
    frag_color = vec4(c, 1.0);
}
