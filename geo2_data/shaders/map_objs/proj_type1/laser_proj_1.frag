#version 410 core

layout(std140) uniform block1
{
    //requires 3 vertices
    vec4 data[256*4];
    //[0] = (v0x, v0y, v1x, v1y)
    //[1] = (v2x, v2y, unused, unused)
    //[2] = inner color
    //[3] = outer color
};

flat in int idx;
in vec3 bary_coord;

out vec4 frag_color;

void main()
{
    frag_color = mix(data[idx+2], data[idx+3], 3*min(bary_coord[0], min(bary_coord[1], bary_coord[2])));
}
