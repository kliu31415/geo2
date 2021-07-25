#version 410 core

//2 vertex line
layout(std140) uniform block1
{
    vec4 data[512*2];
    //[0] = (x, y, z, w)
    //[1] = color
};

flat in int idx;

out vec4 frag_color;

void main()
{
    frag_color = data[idx+1];
}
