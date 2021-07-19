#version 410 core

//requires 4 vertices in a triangle strip

layout(std140) uniform characters
{
    //requires 4 vertices in a triangle strip
    vec4 data[341*3];
    //[0] = (x, y, w, h)
    //[1] = (font size, character (as int), char tex w% of max, char tex h% of max)
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
    //note: the texture has pixels of the form (1, 1, 1, a), where the a is swizzled from the red channel
    frag_color = data[idx+2] * texture(atlas, vec3(tex_coord, character));
    //frag_color = vec4(1);
}
