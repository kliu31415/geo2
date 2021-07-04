#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices
    vec4 data[256*4];
    //[0] = (v0x, v0y, dx01, dy01)
    //[1] = (dx02, dy02, unused, unused)
    //[2] = color 1
    //[3] = color 2
};

flat in int idx;
in vec2 pos;

out vec4 frag_color;

void main()
{
    frag_color = mix(data[idx+2], data[idx+3], 0.5*dot(pos, pos));
}
