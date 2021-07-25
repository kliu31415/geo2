#version 410 core

//4 vertices in a triangle strip

layout(std140) uniform block1
{
    vec4 data[204*5];
    //[0] = (v0, v3)
    //[1] = (% full, 1 - x border size, 1 - y border size, unused)
    //[2] = (full color)
    //[3] = (empty color)
    //[4] = (border color)
};

flat in int idx;

in vec2 normalized_coord;

out vec4 frag_color;

void main()
{
    if(greaterThan(max(normalized_coord, 1 - normalized_coord), data[idx+1].yz) != bvec2(0)) {
        frag_color = data[idx+4];
    } else {
        if((normalized_coord.x - data[idx+1][1]) / (1 - 2*data[idx+1][1]) < 1 - data[idx+1][0])
            frag_color = data[idx+3];
        else
            frag_color = data[idx+2];
    }
}
