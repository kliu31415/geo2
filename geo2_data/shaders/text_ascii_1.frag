#version 410 core

//requires 4 vertices in a triangle strip

layout(std140) uniform characters
{
    //requires 4 vertices in a triangle strip
    vec4 data[341*3];
    //[0] = (x, y, w, h)
    //[1] = (font size, character (as int), (x1%: u16, y1%: u16), (x2%: u16, y2%: u16))
    //[2] = color
};
in vec2 tex_coord;

flat in int idx;

out vec4 frag_color;

//this should only have data in the red channel
uniform sampler2DArray atlas;

void main()
{
    int character = floatBitsToInt(data[idx+1][1]);
    frag_color = data[idx+2];
    frag_color.a *= texture(atlas, vec3(tex_coord, character)).r;
}
