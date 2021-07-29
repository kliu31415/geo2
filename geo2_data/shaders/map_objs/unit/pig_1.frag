#version 410 core

layout(std140) uniform block1
{
    //requires 4 vertices in a triangle strip
    vec4 data[204*5];
    //[0] = (x0, y0, x1, y1)
    //[1] = (x2, y2, x_border_cutoff, y_border_cutoff)
    //[2] = (v0 bary x, v0 bary y, v3 bary x, v3 bary y)
    //[3] = inner color
    //[4] = border color
};

flat in int idx;
in vec2 bary_coord;

out vec4 frag_color;

void main()
{   
    //if(abs(bary_coord.x) > data[idx+1][2] || abs(bary_coord.y) > data[idx+1][3])
    if(greaterThan(abs(bary_coord), data[idx+1].zw) != bvec2(0, 0))
        frag_color = data[idx+4];
    else
        frag_color = data[idx+3];
}
